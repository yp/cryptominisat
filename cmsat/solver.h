/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include <stdint.h>
#include <vector>

#include "constants.h"
#include "solvertypes.h"
#include "solverconf.h"
#include "propengine.h"
#include "searcher.h"
#include "GitSHA1.h"
#include "sqlstats.h"
#include <fstream>

using std::vector;
using std::pair;
using std::string;

class VarReplacer;
class ClauseCleaner;
class Prober;
class Simplifier;
class SCCFinder;
class ClauseVivifier;
class CalcDefPolars;
class SolutionExtender;
class SQLStats;

class Solver : public Searcher
{
    public:
        Solver(const SolverConf& _conf);
        ~Solver();

        //////////
        //External stats
        void fileAdded(const string& filename);

        //////////////////////////////
        //Solving
        lbool solve(const vector<Lit>* _assumptions = NULL);
        void        setNeedToInterrupt();
        vector<lbool>  model;
        lbool   modelValue (const Lit p) const;  ///<Found model value for lit

        //////////////////////////////
        // Problem specification:
        Var  newVar(const bool dvar = true); ///< Add new variable
        bool addClause (const vector<Lit>& ps);  ///< Add clause to the solver
        bool addLearntClause(
            const vector<Lit>& ps
            , const ClauseStats& stats = ClauseStats()
        );

        //////////////////////////
        //Stats
        static const char* getVersion();

        ///Return number of ALL clauses: non-learnt, learnt, bin
        uint64_t getNumLongClauses() const;
        bool     getNeedToDumpLearnts() const;
        bool     getNeedToDumpOrig() const;
        uint32_t getVerbosity() const;
        void     printFullStats();
        void     printClauseStats();
        void     addInPartialSolvingStat();
        size_t   getNumDecisionVars() const;
        size_t   getNumFreeVars() const;

        ///Return number of variables waiting to be replaced
        size_t getNewToReplaceVars() const;
        const Stats& getStats() const;
        uint64_t getNextCleanLimit() const;
        bool     getSavedPolarity(Var var) const;
        uint32_t getSavedActivity(const Var var) const;
        uint32_t getSavedActivityInc() const;

        ///////////////////////////////////
        // State Dumping
        void  dumpBinClauses(const bool alsoLearnt, const bool alsoNonLearnt, std::ostream& outfile) const;
        void  dumpLearnts(std::ostream& os, const uint32_t maxSize); ///<Dump all learnt clauses into file
        void  dumpIrredClauses(std::ostream& os) const; ///<Dump (simplified) irredundant system

        struct SolveStats
        {
            SolveStats() :
                numSimplify(0)
                , nbReduceDB(0)
                , subsBinWithBinTime(0)
                , subsBinWithBin(0)
                , numCallReachCalc(0)
            {}

            SolveStats& operator+=(const SolveStats& other)
            {
                numSimplify += other.numSimplify;
                nbReduceDB += other.nbReduceDB;
                subsBinWithBinTime += other.subsBinWithBinTime;
                subsBinWithBin += other.subsBinWithBin;
                numCallReachCalc += other.numCallReachCalc;

                return *this;
            }

            uint64_t numSimplify;
            uint64_t nbReduceDB;
            double subsBinWithBinTime;
            uint64_t subsBinWithBin;
            uint64_t numCallReachCalc;
        };
        const SolveStats& getSolveStats() const;


        struct ReachabilityStats
        {
            ReachabilityStats() :
                cpu_time(0)
                , numLits(0)
                , dominators(0)
                , numLitsDependent(0)
            {}

            ReachabilityStats& operator+=(const ReachabilityStats& other)
            {
                cpu_time += other.cpu_time;

                numLits += other.numLits;
                dominators += other.dominators;
                numLitsDependent += other.numLitsDependent;

                return *this;
            }

            void print() const
            {
                cout << "c ------- REACHABILITY STATS -------" << endl;
                printStatsLine("c time"
                    , cpu_time
                );

                printStatsLine("c dominator lits"
                    , (double)dominators/(double)numLits*100.0
                    , "% of unknowns lits"
                );

                printStatsLine("c dependent lits"
                    , (double)(numLitsDependent)/(double)numLits*100.0
                    , "% of unknown lits"
                );

                printStatsLine("c avg num. dominated lits"
                    , (double)numLitsDependent/(double)dominators
                );

                cout << "c ------- REACHABILITY STATS END -------" << endl;
            }

            void printShort() const
            {
                cout
                << "c [reach]"
                << " dom lits: " << std::fixed << std::setprecision(2)
                << (double)dominators/(double)numLits*100.0
                << " %"

                << " dep-lits: " << std::fixed << std::setprecision(2)
                << (double)numLitsDependent/(double)numLits*100.0
                << " %"

                << " dep-lits/dom-lits : " << std::fixed << std::setprecision(2)
                << (double)numLitsDependent/(double)dominators

                << " T: " << std::fixed << std::setprecision(2)
                << cpu_time << " s"
                << endl;
            }

            double cpu_time;

            size_t numLits;
            size_t dominators;
            size_t numLitsDependent;
        };

        //Checks
        void checkImplicitPropagated() const;
        void checkStats(const bool allowFreed = false) const;
        void checkImplicitStats() const;

    protected:

        friend class SQLStats;
        SQLStats sqlStats;
        vector<string> fileNamesUsed;

        //Control
        Clause*  newClauseByThread(
            const vector<Lit>& lits
            , const uint32_t glue
        );

        //Attaching-detaching clauses
        virtual void attachClause(const Clause& c);
        virtual void attachBinClause(
            const Lit lit1
            , const Lit lit2
            , const bool learnt
            , const bool checkUnassignedFirst = true
        );
        virtual void attachTriClause(
            const Lit lit1
            , const Lit lit2
            , const Lit lit3
            , const bool learnt
        );
        virtual void detachTriClause(
            const Lit lit1
            , const Lit lit2
            , const Lit lit3
            , const bool learnt
        );
        virtual void detachBinClause(
            const Lit lit1
            , const Lit lit2
            , const bool learnt
        );
        virtual void  detachClause(const Clause& c);
        virtual void  detachModifiedClause(
            const Lit lit1
            , const Lit lit2
            , const uint32_t origSize
            , const Clause* address
        );
        Clause* addClauseInt(
            const vector<Lit>& lits
            , const bool learnt = false
            , const ClauseStats stats = ClauseStats()
            , const bool attach = true
            , vector<Lit>* finalLits = NULL
        );

    private:

        bool addXorClauseInt(const vector<Lit>& lits, bool rhs);
        lbool simplifyProblem();
        SolveStats solveStats;

        /////////////////////
        //Stats
        vector<uint32_t> backupActivity;
        vector<bool>     backupPolarity;
        uint32_t         backupActivityInc;

        /////////////////////
        // Objects that help us accomplish the task
        friend class ClauseAllocator;
        friend class StateSaver;
        friend class SolutionExtender;
        friend class VarReplacer;
        friend class SCCFinder;
        friend class Prober;
        friend class ClauseVivifier;
        friend class Simplifier;
        friend class ClauseCleaner;
        friend class CompleteDetachReatacher;
        friend class CalcDefPolars;
        friend class Searcher;
        friend class XorFinder;
        friend class GateFinder;
        friend class PropEngine;
        Prober              *prober;
        Simplifier          *simplifier;
        SCCFinder           *sCCFinder;
        ClauseVivifier      *clauseVivifier;
        ClauseCleaner       *clauseCleaner;
        VarReplacer         *varReplacer;
        MTRand              mtrand;           ///< random number generator

        /////////////////////////////
        //Renumberer
        vector<Var> outerToInterMain;
        vector<Var> interToOuterMain;
        void renumberVariables();

        /////////////////////////////
        // SAT solution verification
        bool verifyModel() const;                            ///<Verify model[]
        bool verifyBinClauses() const;                       ///<Verify model[] for binary clauses
        bool verifyClauses(const vector<ClOffset>& cs) const; ///<Verify model[] for normal clauses

        ///////////////////////////
        // Clause cleaning
        void fullReduce();
        void clearClauseStats(vector<ClOffset>& clauseset);
        CleaningStats reduceDB();           ///<Reduce the set of learnt clauses.
        struct reduceDBStructGlue
        {
            reduceDBStructGlue(ClauseAllocator* _clAllocator) :
                clAllocator(_clAllocator)
            {}
            ClauseAllocator* clAllocator;

            bool operator () (const ClOffset x, const ClOffset y);
        };
        struct reduceDBStructSize
        {
            reduceDBStructSize(ClauseAllocator* _clAllocator) :
                clAllocator(_clAllocator)
            {}
            ClauseAllocator* clAllocator;

            bool operator () (const ClOffset x, const ClOffset y);
        };
        struct reduceDBStructPropConfl
        {
            reduceDBStructPropConfl(ClauseAllocator* _clAllocator) :
                clAllocator(_clAllocator)
            {}
            ClauseAllocator* clAllocator;

            bool operator () (const ClOffset x, const ClOffset y);
        };

        /////////////////////
        // Data
        SolverConf           conf;
        vector<LitReachData> litReachable;
        void                 calcReachability();
        bool                 needToInterrupt;
        uint64_t             nextCleanLimit;
        uint64_t             nextCleanLimitInc;
        uint32_t             numDecisionVars;
        void setDecisionVar(const uint32_t var);
        void unsetDecisionVar(const uint32_t var);
        size_t               zeroLevAssignsByCNF;
        size_t               zeroLevAssignsByThreads;

        //Main up stats
        Stats sumStats;
        PropStats sumPropStats;
        CleaningStats cleaningStats;
        ReachabilityStats reachStats;

        /////////////////////
        // Clauses
        bool          addClauseHelper(vector<Lit>& ps);
        vector<char>        decisionVar;
        vector<ClOffset>    longIrredCls;          ///< List of problem clauses that are larger than 2
        vector<ClOffset>    longRedCls;          ///< List of learnt clauses.
        uint64_t            irredLits;  ///< Number of literals in non-learnt clauses
        uint64_t            redLits;  ///< Number of literals in learnt clauses
        uint64_t            irredBins;
        uint64_t            redBins;
        uint64_t            irredTris;
        uint64_t            redTris;
        uint64_t            numNewBinsSinceSCC;
        void                reArrangeClauses();
        void                reArrangeClause(ClOffset offset);
        void                checkLiteralCount() const;
        void                printAllClauses() const;
        void                consolidateMem();


        //Subsumtion of bin with bin
        struct WatchSorter {
            bool operator()(const Watched& first, const Watched& second)
            {
                //Anything but clause!
                if (first.isClause())
                    return false;
                if (second.isClause())
                    return true;
                //Now nothing is clause

                if (first.lit1() < second.lit1()) return true;
                if (first.lit1() > second.lit1()) return false;
                if (first.isBinary() && second.isTri()) return true;
                if (first.isTri() && second.isBinary()) return false;
                //At this point either both are BIN or both are TRI


                //Both are BIN
                if (first.isBinary()) {
                    assert(second.isBinary());
                    if (first.learnt() == second.learnt()) return false;
                    if (!first.learnt()) return true;
                    return false;
                }

                //Both are Tri
                assert(first.isTri() && second.isTri());
                if (first.lit2() < second.lit2()) return true;
                if (first.lit2() > second.lit2()) return false;
                if (first.learnt() == second.learnt()) return false;
                if (!first.learnt()) return true;
                return false;
            }
        };
        bool subsumeAndStrengthenImplicit();

        /////////////////
        // Debug
        void testAllClauseAttach() const;
        bool normClauseIsAttached(const ClOffset offset) const;
        void findAllAttach() const;
        bool findClause(const ClOffset offset) const;
        void checkNoWrongAttach() const;
        void printWatchlist(const vec<Watched>& ws, const Lit lit) const;
        void printClauseSizeDistrib();
        ClauseUsageStats sumClauseData(
            const vector<ClOffset>& toprint
            , bool learnt
        ) const;
        void printPropConflStats(
            std::string name
            , const vector<ClauseUsageStats>& stats
        ) const;

        vector<Lit> assumptions;
};

inline void Solver::setDecisionVar(const uint32_t var)
{
    if (!decisionVar[var]) {
        numDecisionVars++;
        decisionVar[var] = true;
    }
}

inline void Solver::unsetDecisionVar(const uint32_t var)
{
    if (decisionVar[var]) {
        numDecisionVars--;
        decisionVar[var] = false;
    }
}

inline bool Solver::getNeedToDumpLearnts() const
{
    return conf.needToDumpLearnts;
}

inline bool Solver::getNeedToDumpOrig() const
{
    return conf.needToDumpOrig;
}

inline uint64_t Solver::getNumLongClauses() const
{
    return longIrredCls.size() + longRedCls.size();
}

inline uint32_t Solver::getVerbosity() const
{
    return conf.verbosity;
}

inline const Searcher::Stats& Solver::getStats() const
{
    return sumStats;
}

inline uint64_t Solver::getNextCleanLimit() const
{
    return nextCleanLimit;
}

inline bool Solver::getSavedPolarity(const Var var) const
{
    return backupPolarity[var];
}

inline uint32_t Solver::getSavedActivity(const Var var) const
{
    return backupActivity[var];
}

inline uint32_t Solver::getSavedActivityInc() const
{
    return backupActivityInc;
}

inline void Solver::addInPartialSolvingStat()
{
    Searcher::addInPartialSolvingStat();
    sumStats += Searcher::getStats();
    sumPropStats += propStats;
}

inline const Solver::SolveStats& Solver::getSolveStats() const
{
    return solveStats;
}

#endif //THREADCONTROL_H