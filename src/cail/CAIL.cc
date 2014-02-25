/*
 * CAIL.cc
 *
 *  Created on: Sep 24, 2013
 *      Author: rhaugh
 *
 *  Details: This source is currently being used to test the modular approach I am taking in implementing CAIL.
 */

#ifndef CAIL_H_
#include <CAIL.h>
#include <vector>
#endif

int main(int argc, char *argv[]){

    using namespace CAIL;

    int opt;
    bool pci_flag, cpu_flag;
    pci_flag = cpu_flag = 0;

    while( ((opt = getopt(argc, argv, "pc")) != -1) ){

        switch(opt){
            case 'p':{
                pci_flag = 1;
                break;
            }
            case 'c':{
                cpu_flag = 1;
                break;
            }
            default:{
                /* Print usage and exit. */
                break;
            }
        }
    }

    if(pci_flag){

        PCIInfo *pci_info = new PCIInfo();

        std::vector<std::string> pci_identifiers;
        pci_info->getPCIValues(pci_identifiers);

        for(std::vector<std::string>::const_iterator it = pci_identifiers.begin(); it != pci_identifiers.end(); ++it){
            std::cout << *it << std::endl;
        }

        delete pci_info;
    }

    if(cpu_flag){

        CPUInfo *cpu_info = new CPUInfo();

        cpu_info->getCPUInformation();

        delete cpu_info;
    }

    return 0;
}
