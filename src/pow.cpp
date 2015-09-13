// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Octocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
	//	Octocoin v2 Diff (fNewDifficultyProtocol): Patch effective @ block 75888
    bool fNewDifficultyProtocol = ((pindexLast->nHeight + 1) >= Params().DiffChangeTarget());

    if (fNewDifficultyProtocol) {
		return GetNextWorkRequired_V2(pindexLast, pblock); 
    }
    else{
    	return GetNextWorkRequired_V1(pindexLast, pblock); 
    }
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

    if (Params().SkipProofOfWorkCheck())
       return true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget)
        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

/* ------ Octocoin Old Difficulty Retargetting ------- */
unsigned int GetNextWorkRequired_V1(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();
    
    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % Params().Interval() != 0)
    {
        if (Params().AllowMinDifficultyBlocks())
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + Params().TargetSpacing()*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % Params().Interval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Octocoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = Params().Interval()-1;
    if ((pindexLast->nHeight+1) != Params().Interval())
        blockstogoback = Params().Interval();

    // Go back by what we want to be nInterval worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);

	/* ------ Octocoin ------- */
    int64_t retargetInterval = Params().Interval();

	// Additional averaging over 4x nInterval window
    if((pindexLast->nHeight + 1) > (4 * retargetInterval)) {
       retargetInterval  *= 4;
	   
	  const CBlockIndex* pindexFirst = pindexLast;
	  for(int i = 0; pindexFirst && i < retargetInterval; i++)
        pindexFirst = pindexFirst->pprev;

       int nActualTimespanLong =
       (pindexLast->GetBlockTime() - pindexFirst->GetBlockTime())/4;

       // Average between short and long windows
       int nActualTimespanAvg = (nActualTimespan + nActualTimespanLong)/2;

       // Apply .25 damping
       nActualTimespan = nActualTimespanAvg + 3 * Params().TargetTimespan();
       nActualTimespan /= 4;
   }

   // The initial settings (difficulty limiter)
   int nActualTimespanMax = Params().TargetTimespan() *1088/1000;
   int nActualTimespanMin = Params().TargetTimespan() *912/1000;
   if (nActualTimespan < nActualTimespanMin) nActualTimespan = nActualTimespanMin;
   if (nActualTimespan > nActualTimespanMax) nActualTimespan = nActualTimespanMax;
	/* ------ Octocoin ------- */

    // Retarget
    uint256 bnNew;
    uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    // Octocoin: intermediate uint256 can overflow by 1 bit
    bool fShift = bnNew.bits() > 235;
    if (fShift)
        bnNew >>= 1;
    bnNew *= nActualTimespan;
    bnNew /= Params().TargetTimespan();
    if (fShift)
        bnNew <<= 1;

    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("Params().TargetTimespan() = %d    nActualTimespan = %d\n", Params().TargetTimespan(), nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

/* ------ Octocoin New Difficulty Retargetting ------- */
unsigned int GetNextWorkRequired_V2(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();
    
    /* ------ Octocoin ------- */
    int64_t retargetInterval = Params().TargetTimespanNEW() / Params().TargetSpacing();
    /* ------ Octocoin ------- */

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % retargetInterval != 0)
    {
        if (Params().AllowMinDifficultyBlocks())
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* TargetSpacing minutes
            // then allow mining of a min-difficulty block.
           	if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + Params().TargetSpacing()*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % retargetInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // OctoCoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = retargetInterval-1;
    if ((pindexLast->nHeight+1) != retargetInterval)
        blockstogoback = retargetInterval;

    // Go back by nInterval
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);

	/* ------ Octocoin ------- */
    // amplitude filter - thanks to daft27 for this code
    nActualTimespan = Params().TargetTimespanNEW() + (nActualTimespan - Params().TargetTimespanNEW())/8;

    if (nActualTimespan < (Params().TargetTimespanNEW() - (Params().TargetTimespanNEW()/4)) ) nActualTimespan = (Params().TargetTimespanNEW() - (Params().TargetTimespanNEW()/4));
    if (nActualTimespan > (Params().TargetTimespanNEW() + (Params().TargetTimespanNEW()/2)) ) nActualTimespan = (Params().TargetTimespanNEW() + (Params().TargetTimespanNEW()/2));
    /* ------ Octocoin ------- */

    // Retarget
    uint256 bnNew;
    uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    // Octocoin: intermediate uint256 can overflow by 1 bit
    bool fShift = bnNew.bits() > 235;
    if (fShift)
        bnNew >>= 1;
    bnNew *= nActualTimespan;
    bnNew /= Params().TargetTimespanNEW();
    if (fShift)
        bnNew <<= 1;

    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("Params().TargetTimespanNEW() = %d    nActualTimespan = %d\n", Params().TargetTimespanNEW(), nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}



