#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iomanip>
using namespace std;

/*
 - 1 honest miner vs M independent selfish miners (no collusion).
 - Target main-chain blocks: N (default 1,000,000).
 - Honest rate r_h, total selfish r_s = 1 - r_h, each selfish has r_s/M.
 - Race rule (per your spec):
   * Selfish miners who are IN the race (lead==1 and published) MINE ONLY on THEIR OWN branch.
   * Honest miner + selfish miners NOT in the race split their hashpower evenly across all race branches.
 - When entering race: the next block RESOLVES and adds TWO blocks:
   (1) the chosen branch's tie block (credited to its owner: honest or that selfish)
   (2) the deciding block (credited to whoever mined it).

 CLI (optional):
   argv[1]=N, argv[2]=M, argv[3]=r_h, argv[4]=seed
 Example:
   ./a.exe 1000000 1 0.75 42
*/

struct SelfishMiner {
    int id;
    long long lead;       // private lead blocks
    long long accepted;   // credited main-chain blocks
    SelfishMiner(): id(0), lead(0), accepted(0) {}
};

struct RaceState {
    bool active;
    // branches: -1 = honest branch; >=0 = selfish miner id that has tie block in race
    vector<int> branches;
    RaceState(): active(false) {}
    void reset() { active = false; branches.clear(); }
};

static inline double urand01() {
    return (double)std::rand() / (double)(RAND_MAX + 1.0); // [0,1)
}

static void clear_leads_except(vector<SelfishMiner>& s, int keep_idx) {
    for (size_t i = 0; i < s.size(); ++i) if ((int)i != keep_idx) s[i].lead = 0;
}

static void find_max_lead(const vector<SelfishMiner>& s, long long& best, vector<int>& idxs) {
    best = 0; idxs.clear();
    for (size_t i = 0; i < s.size(); ++i) if (s[i].lead > best) best = s[i].lead;
    if (best <= 0) return;
    for (size_t i = 0; i < s.size(); ++i) if (s[i].lead == best) idxs.push_back((int)i);
}

int main(int argc, char** argv) {
    // -------- Parameters --------
    long long N = 1000000;  // target confirmed main-chain blocks
    int M = 3;              // # of independent selfish miners
    double r_h = 0.75;      // honest rate in [0.5,1.0] (e.g., r_s=0.25)
    unsigned int seed = (unsigned int)time(NULL);

    if (argc >= 2) N   = atoll(argv[1]);
    if (argc >= 3) M   = atoi(argv[2]);
    if (argc >= 4) r_h = atof(argv[3]);
    if (argc >= 5) seed = (unsigned int)atoi(argv[4]);

    if (M <= 0) { cerr << "Error: M must be >= 1.\n"; return 1; }
    if (r_h < 0.5 || r_h > 1.0) { cerr << "Error: r_h must be in [0.5, 1.0].\n"; return 1; }

    std::srand(seed);

    double r_s = 1.0 - r_h;
    double per_selfish = (M > 0) ? (r_s / M) : 0.0;

    // -------- State --------
    long long public_height = 0;
    long long honest_accepted = 0;

    vector<SelfishMiner> selfish(M);
    for (int i = 0; i < M; ++i) { selfish[i].id = i; selfish[i].lead = 0; selfish[i].accepted = 0; }

    RaceState race;

    // -------- Simulation loop --------
    while (public_height < N) {
        if (!race.active) {
            // Normal mining event
            double x = urand01();
            if (x < r_h) {
                // Honest finds a block
                long long best_lead = 0; vector<int> max_idxs;
                find_max_lead(selfish, best_lead, max_idxs);

                if (best_lead == 0) {
                    // No selfish lead -> honest extends main chain by 1
                    ++public_height; ++honest_accepted;
                } else if (best_lead >= 2) {
                    // Top-lead selfish miner secures 1 block now
                    int chosen_idx = max_idxs.size() == 1 ? max_idxs[0]
                                  : (int)(urand01() * max_idxs.size());
                    if (chosen_idx >= (int)max_idxs.size()) chosen_idx = (int)max_idxs.size() - 1;
                    chosen_idx = max_idxs[chosen_idx];

                    ++public_height;
                    ++selfish[chosen_idx].accepted;
                    selfish[chosen_idx].lead -= 1;
                    clear_leads_except(selfish, chosen_idx);
                } else {
                    // All selfish leads == 1 -> ENTER RACE
                    race.reset(); race.active = true;
                    race.branches.push_back(-1); // honest branch
                    for (int i = 0; i < M; ++i) if (selfish[i].lead == 1) race.branches.push_back(i);
                    // Do NOT add blocks yet; next step resolves with TWO blocks.
                }
            } else {
                // A selfish miner finds a block (withhold)
                if (r_s <= 0.0) continue;
                double y = (x - r_h) / r_s; // in [0,1)
                int idx = (int)(y * M);
                if (idx >= M) idx = M - 1;
                selfish[idx].lead += 1;
            }
        } else {
            // -------- Race resolution with your mining-allocation rule --------
            // Identify which selfish miners are in race
            vector<char> in_race(M, 0);
            int participating = 0;
            for (size_t t = 0; t < race.branches.size(); ++t) {
                int b = race.branches[t];
                if (b >= 0 && b < M) { in_race[b] = 1; participating++; }
            }
            int k = (int)race.branches.size(); // # of race branches (>=2)
            int non_participating = M - participating;

            // Hashpower split:
            // - Each participating selfish miner contributes per_selfish ONLY to its own branch.
            // - "Others" = honest (r_h) + non-participating selfish (non_participating * per_selfish)
            //   are split evenly across k branches.
            double other_hash = r_h + non_participating * per_selfish;

            // 1) Choose which branch gets the deciding block.
            //    Branch prob = owner_hash (0 for honest; per_selfish for a participating selfish)
            //                  + other_hash / k
            double u = urand01();
            double acc = 0.0;
            int chosen_branch = -999; // -1 for honest, >=0 for selfish id
            for (size_t t = 0; t < race.branches.size(); ++t) {
                int b = race.branches[t];
                double owner_hash = (b >= 0 ? per_selfish : 0.0);
                double p_branch = owner_hash + (other_hash / k);
                acc += p_branch;
                if (u < acc) { chosen_branch = b; break; }
            }
            if (chosen_branch == -999) {
                // numeric safety
                chosen_branch = race.branches.back();
            }

            // 2) Determine who mined the deciding block on the chosen branch.
            //    Within that branch:
            //    - With prob owner_hash / p_branch: the owner mined it (if selfish branch).
            //    - Else: from "others", pick honest vs non-participating selfish proportionally.
            int deciding_winner = -1; // -1 = honest; >=0 = selfish id who mined the DECIDING block
            {
                double owner_hash = (chosen_branch >= 0 ? per_selfish : 0.0);
                double p_branch = owner_hash + (other_hash / k);

                double w = urand01() * p_branch;
                if (w < owner_hash && chosen_branch >= 0) {
                    // owner mined deciding block
                    deciding_winner = chosen_branch;
                } else {
                    // mined by "others": select identity proportional to their hash within "others"
                    if (other_hash <= 0.0) {
                        // degenerate (shouldn't happen with r_h>=0.5), fallback to owner if exists
                        deciding_winner = (chosen_branch >= 0 ? chosen_branch : -1);
                    } else {
                        double v = urand01() * other_hash;
                        if (v < r_h) {
                            deciding_winner = -1; // honest
                        } else {
                            // pick one non-participating selfish uniformly (all have per_selfish)
                            double remain = v - r_h;
                            int idx = (int)(remain / per_selfish);
                            if (idx >= non_participating) idx = non_participating - 1;
                            // Map idx-th non-participating to actual miner id
                            int count = -1, chosen_id = -1;
                            for (int i = 0; i < M; ++i) {
                                if (!in_race[i]) {
                                    count++;
                                    if (count == idx) { chosen_id = i; break; }
                                }
                            }
                            if (chosen_id < 0) {
                                // safety: if none found (e.g., all in race), fallback to honest
                                deciding_winner = -1;
                            } else {
                                deciding_winner = chosen_id;
                            }
                        }
                    }
                }
            }

            // 3) Commit TWO blocks:
            //    (a) tie block of chosen_branch -> credited to branch owner
            if (public_height < N) {
                if (chosen_branch == -1) ++honest_accepted;
                else ++selfish[chosen_branch].accepted;
                ++public_height;
            }
            //    (b) deciding block -> credited to deciding_winner
            if (public_height < N) {
                if (deciding_winner == -1) ++honest_accepted;
                else ++selfish[deciding_winner].accepted;
                ++public_height;
            }

            // 4) Reset all private leads and race state
            for (int i = 0; i < M; ++i) selfish[i].lead = 0;
            race.reset();
        }
    }

    // -------- Output --------
    cout.setf(std::ios::fixed);
    cout << setprecision(6);

    cout << "Total main-chain blocks: " << public_height << "\n";
    cout << "Parameters: N=" << N
         << ", M=" << M
         << ", r_h=" << r_h
         << ", r_s=" << r_s
         << ", per_selfish=" << per_selfish
         << ", seed=" << seed << "\n\n";

    double total = (double)public_height;

    cout << "Honest miner blocks: " << honest_accepted
         << "  (" << (total > 0 ? (honest_accepted / total) : 0.0) << ")\n";

    long long sum_selfish = 0;
    for (int i = 0; i < M; ++i) sum_selfish += selfish[i].accepted;

    for (int i = 0; i < M; ++i) {
        cout << "Selfish miner #" << i
             << " blocks: " << selfish[i].accepted
             << "  (" << (total > 0 ? (selfish[i].accepted / total) : 0.0) << ")\n";
    }

    cout << "All selfish miners total: " << sum_selfish
         << "  (" << (total > 0 ? (sum_selfish / total) : 0.0) << ")\n";

    if (honest_accepted + sum_selfish != public_height) {
        cerr << "[Warn] credited != public height; check logic.\n";
    }
    return 0;
}

