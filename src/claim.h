#ifndef CLAIMS
#define CLAIMS

int check_claim_nisynch (const System sys, const int i);
int check_claim_niagree (const System sys, const int i);
int arachne_claim_niagree (const System sys, const int claim_run,
			   const int claim_index);
int arachne_claim_nisynch (const System sys, const int claim_run,
			   const int claim_index);

int prune_claim_specifics (const System sys);
void add_claim_specifics (const System sys, const Claimlist cl, const
			  Roledef rd);

#endif
