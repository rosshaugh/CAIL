/*
 * CAILCPUInfo.h
 *
 *  Created on: 15 Nov 2013
 *      Author: rossh
 */

#ifndef CAILCPUINFO_H_
#define CAILCPUINFO_H_

#include <CAILGlobals.h>
#include <CAILException.h>
#include <bitset>
#include <sstream>

namespace CAIL{

    class CPUInfo{
    public:
        CPUInfo();
        ~CPUInfo();

        /* Relevant registers */
        typedef struct {
            unsigned int eax;
            unsigned int ebx;
            unsigned int ecx;
            unsigned int edx;
        } cpuinfo_cpuid_container;

        void getCPUInformation();

    private:
        void cpuid(unsigned int);
        void printRegisterValues();
        void buildVendorIdentifierString();
        const std::string & getVendorIdentifierString();

        void initialise();
        void getBasicProcessorInformation();
        void getBasicFeatureInformation();
        std::string getProcessorTypeString(unsigned short &);

        void getExtendedProcessorInformation();
        void getExtendedBrandString();
        void trimBrandedString(std::string &);

        cpuinfo_cpuid_container m_regs;
        unsigned int m_largest_supported_func_basic, m_largest_supported_func_ext;
        std::string m_vendor_id_str;

        enum CPUID_MODIFIERS{
        	VEND_ID_LARGE_STD_FUNC = 0x0,
        	FEATURE_INFORMATION = 0x01,
        	EXTENDED_INFORMATION = 0x80000000,
        	EXTENDED_BRAND_STRING_ONE = 0x80000002,
        	EXTENDED_BRAND_STRING_TWO = 0x80000003,
        	EXTENDED_BRAND_STRING_THREE = 0x80000004
        };
    };
}

#endif /* CAILCPUINFO_H_ */
