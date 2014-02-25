/*
 * CAILCPUInfo.cc
 *
 *  Created on: 15 Nov 2013
 *      Author: rhaugh
 */

#ifndef CAILCPUINFO_H_
#include <CAILCPUInfo.h>
#endif

namespace CAIL{

    CPUInfo::CPUInfo() : m_largest_supported_func_basic(0), m_largest_supported_func_ext(0){

    	initialise();
    }

    CPUInfo::~CPUInfo(){

    	/**/
    }

    inline void CPUInfo::cpuid(unsigned int cpuid_modifier){

        asm volatile(
                "movl %4, %%eax; cpuid;"
        	    "movl %%eax, %0; movl %%ebx, %1; movl %%ecx, %2; movl %%edx, %3;"
                : "=r" (m_regs.eax), "=r" (m_regs.ebx), "=r" (m_regs.ecx), "=r" (m_regs.edx)
                : "g" (cpuid_modifier)
                : "%eax", "%ebx", "%ecx", "%edx"
                );
    }

    void CPUInfo::printRegisterValues(){

    	std::bitset<sizeof(unsigned int)*BYTE_SIZE> eax_bits (m_regs.eax);
    	std::bitset<sizeof(unsigned int)*BYTE_SIZE> ebx_bits (m_regs.ebx);
    	std::bitset<sizeof(unsigned int)*BYTE_SIZE> ecx_bits (m_regs.ecx);
    	std::bitset<sizeof(unsigned int)*BYTE_SIZE> edx_bits (m_regs.edx);

    	std::cout << "EAX==" << eax_bits << "\nEBX==" << ebx_bits << "\nECX==" \
    			<< ecx_bits << "\nEDX==" << edx_bits << std::endl;
    }

    void CPUInfo::initialise(){

    	getBasicProcessorInformation();
    	getExtendedProcessorInformation();
    	getExtendedBrandString();
    	getBasicFeatureInformation();
    }

    void CPUInfo::getCPUInformation(){

    	/**/
    }

    void CPUInfo::buildVendorIdentifierString(){

        std::ostringstream vend_id_builder;

        vend_id_builder << (char)(m_regs.ebx&0xff) << (char)((m_regs.ebx>>8)&0xff) \
        				<< (char)((m_regs.ebx>>16)&0xff) << (char)((m_regs.ebx>>24)&0xff);

        vend_id_builder << (char)(m_regs.edx&0xff) << (char)((m_regs.edx>>8)&0xff) \
                		<< (char)((m_regs.edx>>16)&0xff) << (char)((m_regs.edx>>24)&0xff);

        vend_id_builder << (char)(m_regs.ecx&0xff) << (char)((m_regs.ecx>>8)&0xff) \
                		<< (char)((m_regs.ecx>>16)&0xff) << (char)((m_regs.ecx>>24)&0xff);

        m_vendor_id_str = vend_id_builder.str();
    }

    const std::string & CPUInfo::getVendorIdentifierString(){

    	return m_vendor_id_str;
    }

    void CPUInfo::getBasicProcessorInformation(){

    	std::cout << "CPUInfo::getBasicProcessorInformation()" << std::endl;

    	cpuid(VEND_ID_LARGE_STD_FUNC);
    	buildVendorIdentifierString();
    	m_largest_supported_func_basic = m_regs.eax;

    	std::cout << getVendorIdentifierString() << std::endl;
    }

    void CPUInfo::getBasicFeatureInformation(){

        std::cout << "CPUInfo::getBasicFeatureInformation()" << std::endl;

        unsigned short stepping_id, model_number, family_code, processor_type, extended_model, extended_family;
        stepping_id = model_number = family_code = processor_type = extended_model = extended_family = 0;
        std::ostringstream ecx_feature_builder;

        cpuid(FEATURE_INFORMATION);
        printRegisterValues();

        stepping_id = ((m_regs.eax)&0x0f);
        model_number = ((m_regs.eax>>4)&0x1f);
        family_code = ((m_regs.eax>>8)&0x1f);
        processor_type = ((m_regs.eax>>12)&0x03);
        extended_model = ((m_regs.eax>>14)&0x1f);
        extended_family = ((m_regs.eax>>18)&0x1ff);

        std::cout << "stepping_id: \t\t" << stepping_id << std::endl;
        std::cout << "model_number: \t\t" << model_number << std::endl;
        std::cout << "family_code: \t\t" << family_code << std::endl;
        std::cout << "processor_type: \t" << getProcessorTypeString(processor_type) << std::endl;
        std::cout << "extended_model: \t" << extended_model << std::endl;
        std::cout << "extended_family: \t" << extended_family << std::endl;

        if( (m_regs.edx&0x01) ){
            ecx_feature_builder << "FPU: Floating-point Unit On-Chip\n";
        }
        if( ((m_regs.edx>>1)&0x01) ){
            ecx_feature_builder << "VME: Virtual Mode Extension \n";
        }
        if( ((m_regs.edx>>2)&0x01) ){
            ecx_feature_builder << "DE: Debugging Extension\n";
        }
        if( ((m_regs.edx>>3)&0x01) ){
            ecx_feature_builder << "PSE: Page Size Extension\n";
        }
        if( ((m_regs.edx>>4)&0x01) ){
            ecx_feature_builder << "TSC: Time Stamp Counter\n";
        }
        if( ((m_regs.edx>>5)&0x01) ){
            ecx_feature_builder << "MSR: Model Specific Registers\n";
        }
        if( ((m_regs.edx>>6)&0x01) ){
            ecx_feature_builder << "PAE: Physical Address Extension\n";
        }
        if( ((m_regs.edx>>7)&0x01) ){
            ecx_feature_builder << "MCE: Machine-Check Exception\n";
        }
        if( ((m_regs.edx>>8)&0x01) ){
            ecx_feature_builder << "CX8: CMPXCHG8 Instruction\n";
        }
        if( ((m_regs.edx>>9)&0x01) ){
            ecx_feature_builder << "APIC: On-chip APIC Hardware\n";
        }
        if( ((m_regs.edx>>10)&0x01) ){
            ecx_feature_builder << "Reserved\n";
        }
        if( ((m_regs.edx>>11)&0x01) ){
            ecx_feature_builder << "SEC: Fast System Call\n";
        }
        if( ((m_regs.edx>>12)&0x01) ){
            ecx_feature_builder << "MTRR: Memory Type Range Registers\n";
        }
        if( ((m_regs.edx>>13)&0x01) ){
            ecx_feature_builder << "PGE: Page Global Enable\n";
        }
        if( ((m_regs.edx>>14)&0x01) ){
            ecx_feature_builder << "MCA: Machine-Check Architecture\n";
        }
        if( ((m_regs.edx>>15)&0x01) ){
            ecx_feature_builder << "CMOV: Conditional Move Instruction\n";
        }
        if( ((m_regs.edx>>16)&0x01) ){
            ecx_feature_builder << "PAT: Page Attribute Table\n";
        }
        if( ((m_regs.edx>>17)&0x01) ){
            ecx_feature_builder << "PSE-36: 36-bit Page Size Extension\n";
        }
        if( ((m_regs.edx>>18)&0x01) ){
            ecx_feature_builder << "PSN: Processor serial number is present and enabled\n";
        }
        if( ((m_regs.edx>>19)&0x01) ){
            ecx_feature_builder << "CLFSH: CLFLUSH Instruction\n";
        }
        if( ((m_regs.edx>>20)&0x01) ){
            ecx_feature_builder << "Reserved\n";
        }
        if( ((m_regs.edx>>21)&0x01) ){
            ecx_feature_builder << "DS: Debug Store\n";
        }
        if( ((m_regs.edx>>22)&0x01) ){
            ecx_feature_builder << "ACPI: Thermal Monitor and Software Controlled Clock Facilities\n";
        }
        if( ((m_regs.edx>>23)&0x01) ){
            ecx_feature_builder << "MMX: MMX technology\n";
        }
        if( ((m_regs.edx>>24)&0x01) ){
            ecx_feature_builder << "FXSR: FXSAVE and FXSTOR Instructions\n";
        }
        if( ((m_regs.edx>>25)&0x01) ){
            ecx_feature_builder << "SSE: Streaming SIMD Extensions\n";
        }
        if( ((m_regs.edx>>26)&0x01) ){
            ecx_feature_builder << "SSE2: Streaming SIMD Extensions 2\n";
        }
        if( ((m_regs.edx>>27)&0x01) ){
            ecx_feature_builder << "SS: Self-Snoop\n";
        }
        if( ((m_regs.edx>>28)&0x01) ){
            ecx_feature_builder << "HTT: Multi-Threading\n";
        }
        if( ((m_regs.edx>>29)&0x01) ){
            ecx_feature_builder << "TM: Thermal Monitor\n";
        }
        if( ((m_regs.edx>>30)&0x01) ){
            ecx_feature_builder << "Reserved\n";
        }
        if( ((m_regs.edx>>31)&0x01) ){
            ecx_feature_builder << "PBE: Pending Break Enable\n";
        }

        std::cout << ecx_feature_builder.str() << std::endl;
    }

    std::string CPUInfo::getProcessorTypeString(unsigned short &proc_type){

        std::string proc_type_str;

        switch(proc_type){
            case(0x00):{
                proc_type_str = "Original OEM Processor";
                break;
            }
            case(0x01):{
                proc_type_str = "Overdrive Processor";
                break;
            }
            case(0x02):{
                proc_type_str = "Dual Processor";
                break;
            }
            default:{
                break;
            }
        }

        return proc_type_str;
    }

    void CPUInfo::getExtendedProcessorInformation(){

    	std::cout << "CPUInfo::getExtendedProcessorInformation()" << std::endl;

    	cpuid(EXTENDED_INFORMATION);
    	m_largest_supported_func_ext = m_regs.eax;
    }

    void CPUInfo::getExtendedBrandString(){

    	std::cout << "CPUInfo::getExtendedBrandString()" << std::endl;
        std::ostringstream vend_id_builder;
        std::string brand_str;

    	cpuid(EXTENDED_BRAND_STRING_ONE);

    	vend_id_builder << (char)(m_regs.eax&0xff) << (char)((m_regs.eax>>8)&0xff) \
    	                        << (char)((m_regs.eax>>16)&0xff) << (char)((m_regs.eax>>24)&0xff);

        vend_id_builder << (char)(m_regs.ebx&0xff) << (char)((m_regs.ebx>>8)&0xff) \
                        << (char)((m_regs.ebx>>16)&0xff) << (char)((m_regs.ebx>>24)&0xff);

        vend_id_builder << (char)(m_regs.ecx&0xff) << (char)((m_regs.ecx>>8)&0xff) \
                        << (char)((m_regs.ecx>>16)&0xff) << (char)((m_regs.ecx>>24)&0xff);

        vend_id_builder << (char)(m_regs.edx&0xff) << (char)((m_regs.edx>>8)&0xff) \
                        << (char)((m_regs.edx>>16)&0xff) << (char)((m_regs.edx>>24)&0xff);

    	cpuid(EXTENDED_BRAND_STRING_TWO);

    	vend_id_builder << (char)(m_regs.eax&0xff) << (char)((m_regs.eax>>8)&0xff) \
    	                        << (char)((m_regs.eax>>16)&0xff) << (char)((m_regs.eax>>24)&0xff);

        vend_id_builder << (char)(m_regs.ebx&0xff) << (char)((m_regs.ebx>>8)&0xff) \
                        << (char)((m_regs.ebx>>16)&0xff) << (char)((m_regs.ebx>>24)&0xff);

        vend_id_builder << (char)(m_regs.ecx&0xff) << (char)((m_regs.ecx>>8)&0xff) \
                        << (char)((m_regs.ecx>>16)&0xff) << (char)((m_regs.ecx>>24)&0xff);

        vend_id_builder << (char)(m_regs.edx&0xff) << (char)((m_regs.edx>>8)&0xff) \
                        << (char)((m_regs.edx>>16)&0xff) << (char)((m_regs.edx>>24)&0xff);

    	cpuid(EXTENDED_BRAND_STRING_THREE);

    	vend_id_builder << (char)(m_regs.eax&0xff) << (char)((m_regs.eax>>8)&0xff) \
    	                << (char)((m_regs.eax>>16)&0xff) << (char)((m_regs.eax>>24)&0xff);

        vend_id_builder << (char)(m_regs.ebx&0xff) << (char)((m_regs.ebx>>8)&0xff) \
                        << (char)((m_regs.ebx>>16)&0xff) << (char)((m_regs.ebx>>24)&0xff);

        vend_id_builder << (char)(m_regs.ecx&0xff) << (char)((m_regs.ecx>>8)&0xff) \
                        << (char)((m_regs.ecx>>16)&0xff) << (char)((m_regs.ecx>>24)&0xff);

        vend_id_builder << (char)(m_regs.edx&0xff) << (char)((m_regs.edx>>8)&0xff) \
                        << (char)((m_regs.edx>>16)&0xff) << (char)((m_regs.edx>>24)&0xff);

        brand_str = vend_id_builder.str();
        trimBrandedString(brand_str);

        std::cout << brand_str << std::endl;
    }

    void CPUInfo::trimBrandedString(std::string &brand){

        unsigned short pos = 0;
        while(pos<brand.length()){
            if(brand[pos]==' '){
                pos++;
            }
            else{
                break;
            }
        }
        brand.erase(0,pos);
    }
}
