//
// Created by Katie Barr (EI) on 23/11/2017.
//

#include "PhaseScaffolder.h"



PhaseScaffolder::PhaseScaffolder(SequenceGraph & sg): sg(sg), mapper(sg){
}

void PhaseScaffolder::load_mappings(std::string r1_filename, std::string r2_filename, std::string fasta_filename, uint64_t max_mem_gb, std::string to_map=""){

    mapper.map_reads(r1_filename, r2_filename, fasta_filename, prm10x, max_mem_gb);
    std::cout << "Mapped " << mapper.read_to_node.size() << " reads to " <<  mapper.reads_in_node.size() << "nodes" << std::endl;
    if (to_map.size()>0){
        std::cout << "Writting mappings to disk" << std::endl;
        mapper.save_to_disk(to_map);
    }
}


void PhaseScaffolder::output_bubbles(std::string bubble_filename) {

        std::ofstream out2(bubble_filename);
//find each component of gfa
    std::cout << "Finding components" << std::endl;
    auto components = sg.connected_components();
// this finds 2 components for test graph...
    std::cout << "Found " << components.size() << " connected components " << std::endl;
    int counter = 0;
    int counter2 = 0;
    for (auto component:components) {
        if (component.size() >= 6) {
            auto bubbles = sg.find_bubbles(component);

            if (bubbles.size() > 1) {
                counter2+=1;
                for (auto bubble:bubbles) {

                    for (auto bubble_c:bubble) {
                        out2 << ">" << sg.oldnames[bubble_c] << "_" << counter << std::endl
                             << sg.nodes[bubble_c].sequence << std::endl;
                    }

                }

            }

            counter += 1;
        }
    }
    std::cout << counter << " components with enough contigs to be phaseable, " << counter2 << " contained more tan 1 bubble, i.e. are phaseable \n";
}

void PhaseScaffolder::phase_components() {

//find and phase each component of gfa
    auto components = sg.connected_components();
// this finds 2 components for test graph...
    std::cout << "Found " << components.size() << " connected components " << std::endl;
    for (auto component:components) {
        HaplotypeScorer hs;
        if (component.size() >= 6) {
        /*    for (auto c:component){
                std::cout << sg.oldnames[c] << " ";
            }
std::cout << std::endl;*/
// should
            auto bubbles = sg.find_bubbles(component);
            /*for (auto b: bubbles){
                for (auto c:b){
                    std::cout << sg.oldnames[c] << " ";
                }
                std::cout << std::endl;
            }*/
            hs.find_possible_haplotypes(bubbles);
            std::cout << "mapper.reads_in_node.size()  " << mapper.reads_in_node.size() << std::endl;

            hs.count_barcode_votes(mapper);
            hs.decide_barcode_haplotype_support();
// now have mappings and barcode support
            if (hs.barcode_haplotype_mappings.size() > 0) {
                hs.score_haplotypes(sg.oldnames);
                std::cout << "scored haplotypes " << hs.success << std::endl;
            }
        }
    }
}