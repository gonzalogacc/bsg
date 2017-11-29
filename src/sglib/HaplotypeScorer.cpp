//
// Created by Katie Barr (EI) on 14/11/2017.
//

#include "HaplotypeScorer.hpp"



/**
 *
 *
 * @brief
 * sum each barcode's score for each haplotype
 * @return
 * Returns a vector containing: total records generated by the factory,
 * total records after filtering,
 * total file records present in the file and the total number of records after filtering.
 */

void HaplotypeScorer::decide_barcode_haplotype_support(){

    int support;
    int haplotypes_supported = 0;

    std::cout << "Calculating barcode haplotype" <<std::endl;
    for (auto &mapping:barcode_node_mappings) {
        if (mapping.second.size() > 1) {
            std::vector<sgNodeID_t> nodes;
            std::vector<int> scores;
            for (auto n: mapping.second) {
                nodes.push_back(n.first);
                scores.push_back(n.second);
            }
            if (*std::max_element(scores.begin(), scores.end()) > 1) {
                for (int i = 0; i < haplotype_ids.size(); i++) {
                    std::vector<sgNodeID_t> nodes_in_haplotype;
                    std::vector<sgNodeID_t> h;
                    h = haplotype_ids[i];
                    // find all nods in each haplotype that this barcode maps to
                    for (auto n1: nodes) {
                        if (std::find(h.begin(), h.end(), n1) != h.end()) {
                            nodes_in_haplotype.push_back(n1);
                        }
                    }
                    // somewhat arbitrary rule to decide if the barcode supports a haplotype enough
                    if (nodes_in_haplotype.size() >= (nodes.size() / 2) && nodes_in_haplotype.size() > 1) {
                        support = 0;
                        for (auto a: nodes_in_haplotype) {
                            support += mapping.second[a];
                        }
                        barcode_haplotype_mappings[mapping.first][i] = support;
                        haplotypes_supported += 1;
                    } else {
                        unused_barcodes.push_back(mapping.first);
                    }
                }
            }

        } else {
            unused_barcodes.push_back(mapping.first);
        }
        haplotypes_supported = 0;

    }


    std::cout << "Calculated haplotype support for each barcode, " << barcode_haplotype_mappings.size() <<  std::endl;

}


/**
 *
 *
 * @brief
 * Map reads to graph, sum up matches for each comntig mapped to by each barcode  barcode
 * @return
 * Load read files 1 and 2, populating map of id to barcode (this should be a vector the same as read to node but either my input reads are dodgy or there's something else wrong)
 * find mappings for each node in the bubble contigs. for each barcode, sum up the number of mappings for each node that a read with that barcode maps to
 *
 */
void HaplotypeScorer::count_barcode_votes(PairedReadMapper & mapper){
    std::cout << "Mapped " << mapper.read_to_node.size() << " reads to " <<  mapper.reads_in_node.size() << "nodes" << std::endl;
    std::cout << "NOw counting barcode votes... " << std::endl;
    std::cout << "Haplotype nodes size: " << haplotype_nodes.size() << std::endl;
    std::cout << "mapper.reads_in_node.size()  " << mapper.reads_in_node.size() <<std::endl;
    int counter = 0;
    for (auto r: mapper.reads_in_node){
        std::cout << "Node " << counter << " contsins " << r.size() <<" mappings " <<std::endl;
        counter += 1;
    }
    for (auto &node:haplotype_nodes){
        std::cout << "Node: " <<node << " mappings " << mapper.reads_in_node[node].size() << std::endl;
        for (auto &mapping:mapper.reads_in_node[node>0?node:-node]){
            auto barcode = mapper.read_to_tag[mapping.read_id];
             barcode_node_mappings[barcode][node] += mapping.unique_matches;
            counter += 1;
        }
    }
    std::cout << "counted " << counter << " votes for " << barcode_node_mappings.size() << "barcodes";
};

/**
 *
 *
 * @brief
 * Decide winning phasing
 * @return
 * Calculate, for each haplotype, a number of metrics that represent how well the 10x reads support that phasing
 * loop over each barcode in turn, then each phasing pair
 * \todo decide criteria for winning haploype
 *
 */
int HaplotypeScorer::score_haplotypes(std::vector<std::string> oldnames) {
    auto number_haplotypes = haplotype_ids.size();

    std::cout << "Finding most supported of " << number_haplotypes<< " possible haplotypes"<<std::endl;
    //initialize score arrays- index is haplotype index
    std::vector<int > haplotype_support;
    std::vector<int > haplotype_not_support;
    std::vector<int> haplotype_overall_support;
    haplotype_support.resize(number_haplotypes);
    haplotype_not_support.resize(number_haplotypes);
    haplotype_overall_support.resize(number_haplotypes);
    std::map<std::pair<int, int>, int> hap_pair_not_support;
    std::map<std::pair<int, int>, int> hap_pair_support;
    std::map<std::pair<int, int>, int> hap_pair_support_total_score;
    prm10xTag_t barcode;
    for (auto &bm: barcode_haplotype_mappings) {
        barcode = bm.first;
        std::vector<int> winners = winner_for_barcode(barcode); // ideally should be length 1
        for (auto winner:winners){
            int pair = haplotype_ids.size() - 1 - winner;
            haplotype_support[winner] += 1;

            hap_pair_support[std::make_pair(winner, pair)] += 1;
            haplotype_barcode_agree[winner][barcode] += bm.second[winner];
            haplotype_barcode_disagree[winner][barcode] += bm.second[pair];
        }
        for (int hap = 0; hap < number_haplotypes/ 2; hap++) {
            // pair = len(self.list_of_possible_haplotypes) -1 -haplotype
            auto pair = number_haplotypes - 1 - hap;
            if (bm.second.find(hap) != bm.second.end()) {
                haplotype_overall_support[hap] += bm.second[hap];
                hap_pair_support_total_score[std::make_pair(hap, pair)] += bm.second[hap];
            }

            if (bm.second.find(pair) != bm.second.end()) {
                haplotype_overall_support[pair] += bm.second[pair];
                hap_pair_support_total_score[std::make_pair(hap, pair)] += bm.second[pair];
            }
            if (bm.second.find(hap) == bm.second.end()) {
                haplotype_not_support[hap] += 1;
            }
            if (bm.second.find(pair) == bm.second.end()) {
                haplotype_not_support[pair] += 1;
            }
            if (bm.second.find(hap) == bm.second.end() and bm.second.find(pair) == bm.second.end()) {
                hap_pair_not_support[std::make_pair(hap, pair)] += 1;

            }
        }
    }
    // this gives value of max support, not index, must be better way

    auto winner_index = std::max_element(haplotype_support.begin(), haplotype_support.end());
    std::cout << "hap " << *winner_index << " winning contigs: " << std::endl;

    std::vector<int> support_winner;

    for (int h = 0; h < haplotype_ids.size(); h++) {
        if (haplotype_support[h] == *winner_index) {
            std::cout << "Winner: " << h << std::endl;
            support_winner.push_back(h);
        }
    }
    std::cout << "hap " << *winner_index << " winning contigs: " << std::endl;

    for (auto w: support_winner){
       for (auto h:haplotype_ids[w]){

           std::cout <<" c " << oldnames[h] << " ";

       }
       std::cout << std::endl;

    }

    return 0;
}

/**
 *
 *
 * @brief
 * Find haplotype that a barcode votes for
 * * @return
 * loop over barcode aplotype mappings to find maximum support
 * if more than2 have same support both are returned
 * \todo decide heuristics to vote for winner
 */
std::vector<int>  HaplotypeScorer::winner_for_barcode(prm10xTag_t barcode){
    int max=0;
    std::vector<int> winners;
    for (auto h:barcode_haplotype_mappings[barcode]){
        if (h.second > max){
            max = h.second;
        }
    }
    //TODO: DECIDE CRITERIA FOR MINIMUM SUPPORT
    for (auto h:barcode_haplotype_mappings[barcode]){
        if (h.second == max){
            winners.push_back(h.first);
        }
    }
    return winners;
}


void HaplotypeScorer::find_possible_haplotypes(std::vector<std::vector<sgNodeID_t >> bubbles){
    // algorithm: https://stackoverflow.com/questions/1867030/combinations-of-multiple-vectors-elements-without-repetition
    size_t P = 1;
    auto N =bubbles.size();
    for(size_t i=0;i<N;++i) {
        P *= bubbles[i].size();
        haplotype_nodes.insert(bubbles[i].begin(), bubbles[i].end());
    }
    std::vector<std::vector<sgNodeID_t >> haps;
    std::cout << P << " combinations to generate from " << N << " bubbles " << std::endl;
    for (size_t m=0; m < P; m++ ) {
        // this should hold the index to take from each bubble
        std::vector<size_t> indices(N);
        std::vector<sgNodeID_t > bubble;
        size_t m_curr = m;
        for (size_t i = 0; i < N; ++i) {
            indices[i] = m_curr% bubbles[i].size();
            bubble.push_back(bubbles[i][indices[i]]);
            m_curr /= bubbles[i].size();
        }

        haplotype_ids.push_back(bubble);
    }
    std::cout << haplotype_ids.size() << " haplotypes  generated " << std::endl;

    std::cout << "Haplotype nodes size: " << haplotype_nodes.size() << std::endl;

    this->haplotype_nodes = haplotype_nodes;
    std::cout << "Haplotype nodes size: " << haplotype_nodes.size() << std::endl;

}