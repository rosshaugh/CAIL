/*
 *  CAILPCIInfo.cc
 *
 *  Created on: Aug 23, 2013
 *      Author: rhaugh
 *
 *
 *  TODO:
 *  1) Currently, I am only looking at PCI devices. I need to study the implementation of PCI express if it is to be supported.
 *  2) Rework the flow.
 *      2a) Currently, each PCI addr is read in sequence, as provided by the OS, and the devices are identified in that order. However, the
 *          pci.ids data is ordered by device_id. As such, it would make much more sense to gather all device_ids, order them correctly, and
 *          then retrieve the identifer string from pci.ids
 *      2b) Remove the std::string::find implementation of getPCIIdentifierString() and do it manually. This will negate the need for a std::string
 *          version of the pci.ids file contents.
 */

#ifndef CAILPCIINFO_H_
#include <CAILPCIInfo.h>
#endif

namespace CAIL{

    /***********************************************************************************************************************/

    PCIInfo::DataBuffer::DataBuffer(size_t size,  const char *file_name) : m_buffer(0), m_size(size), m_file_name(file_name), data_fp(0){

        initialiseBuffer();
    }

    PCIInfo::DataBuffer::DataBuffer(const char *file_name) :  m_buffer(0), m_size(0), m_file_name(file_name), data_fp(0){

        /**/
    }

    size_t PCIInfo::DataBuffer::getSize(){

        return m_size;
    }

    PCIInfo::DataBuffer::~DataBuffer(){

        if(m_buffer){
            delete [] m_buffer;
        }
    }

    void PCIInfo::DataBuffer::initialiseBuffer(){

        if( (m_buffer==0) ){
            m_buffer = new unsigned char[(m_size+1)]();
            memset(m_buffer, '\0', sizeof(m_buffer));
        }
    }

    void PCIInfo::DataBuffer::populateBuffer(){

        if( ((data_fp = fopen(m_file_name.c_str(), "rb")) == NULL) ){
            throw CAILException("fopen: %s while opening %s", strerror(errno), m_file_name.c_str());
        }

        if( (m_size==0) ){
            fseek(data_fp, 0, SEEK_END);
            m_size = ftell(data_fp);
            rewind(data_fp);

            initialiseBuffer();
        }

        if( (fread(m_buffer, sizeof(unsigned char), m_size, data_fp) != m_size) ){
            /* TODO: check ferror and feof. */
            throw CAILException("fread: %s while reading %s", strerror(errno), m_file_name.c_str());
        }

        fclose(data_fp);
    }

    unsigned char *PCIInfo::DataBuffer::getBuffer(){

        return m_buffer;
    }

    /***********************************************************************************************************************/

    PCIInfo::PCIInfo(){

        initialise();
    }

    PCIInfo::~PCIInfo(){

        if(pci_ids_data){
            delete pci_ids_data;
        }
    }

    void PCIInfo::initialise(){

        // TODO: These need to be dynamically set based on what system this program is being executed on.
        m_pci_dir = "/sys/bus/pci/devices/";
        m_pcix_dir = "/sys/bus/pci_express/devices/";
        m_config_file = "/config";

        const char * linux_pci_id_file_loc = "/usr/share/misc/pci.ids";
        const char * rhel_pci_id_file_loc = "/usr/share/hwdata/pci.ids";

        /* TODO: Need to consider multiple possible locations. Also, get this from web if not available / out of date? */
        struct stat st_buffer;
        if(stat(linux_pci_id_file_loc, &st_buffer) == 0){
            m_pci_id_file_loc = linux_pci_id_file_loc;
        }
        else{
            m_pci_id_file_loc = rhel_pci_id_file_loc;
        }

        /* All we are really interested in so far. */
        /* TODO: Need to check header type value to determine offset of subsystem[id|vendorid] */
        m_VENDOR_ID_POS = 0x00;
        m_SUB_VENDOR_ID_POS = 0x2c;
        m_DEVICE_ID_POS = 0x02;
        m_SUB_DEVICE_ID_POS = 0x2e;
        m_CLASS_CODE_POS = 0x0b;
        m_SUBCLASS_CODE_POS = 0x0a;
        m_PROGIF_CODE_POS = 0x09;
        m_REVISION_ID_POS = 0x08;
        m_CONFIG_AMOUNT = 64;

        try{
            getPCIIdenfifierFileContents();
        }
        catch(CAILException &ex){
            std::cout << "CAILException: " << ex.getExceptionText() << std::endl;
        }
    }

    void PCIInfo::getPCIIdenfifierFileContents(){

        pci_ids_data = new DataBuffer(m_pci_id_file_loc.c_str());
        pci_ids_data->populateBuffer();

        /* TODO: 2x memory consumption here. Done for convenience. Need to revise. Also, dat cast. */
        m_pci_id_file_data.assign(reinterpret_cast<char*>(pci_ids_data->getBuffer()));
    }

    void PCIInfo::getPCIValues(std::vector<std::string> &pci_identifiers){

        try{
            getPCIDirectories(pci_identifiers);
        }
        catch(CAILException &ex){
            std::cout << "CAILException: " << ex.getExceptionText() << std::endl;
        }
    }

    void PCIInfo::getPCIDirectories(std::vector<std::string> &pci_identifiers){

        DIR *pci_base;
        struct dirent *de;
        struct stat st;

        std::string pci_path, pci_dev_str;

        /* Set path and change directory. */
        if( ((pci_base = opendir(m_pci_dir.c_str())) == NULL) ){
            throw CAILException("opendir: %s (%s)", strerror(errno), m_pci_dir.c_str());
        }

        if( (chdir(m_pci_dir.c_str())!=0) ){
            closedir(pci_base);
            throw CAILException("chdir: %s (%s)", strerror(errno), m_pci_dir.c_str());
        }

        /* Each directory stored under m_pci_dir corresponds to a PCI device installed.
         * Each directory is read in turn and the relevant information is retrieved from the PCI device's configuration space. */
        while( ((de = readdir(pci_base)) != NULL) ){

            /* Now stat the file to get more information */
            if( (stat(de->d_name, &st) == -1) ){
                // Need some sort of logging mechanism here. (stderr for now?)
                perror("stat");
                continue;
            }

            /* Only get dirs and also skip current and previous dirs (. and ..) */
            if( !(S_ISDIR(st.st_mode)) || !(de->d_name[0]>='0') ){
                continue;
            }

            /* Build PCI path to configuration file. */
            pci_path += m_pci_dir;
            pci_path += de->d_name;
            pci_path += m_config_file;

            try{
                DataBuffer buff(m_CONFIG_AMOUNT, pci_path.c_str());
                buff.populateBuffer();
                /* Construct full string here. */
                pci_dev_str += de->d_name; pci_dev_str += ": ";
                pci_dev_str += getPCIDeviceString(buff);

                pci_identifiers.push_back(pci_dev_str);
            }
            catch(CAILException &ex){
                closedir(pci_base);
                throw ex;
            }

            pci_dev_str.clear();
            pci_path.clear();
        }

        closedir(pci_base);
    }

    std::string PCIInfo::getPCIDeviceString(DataBuffer &buff){

        std::string pci_dev_str;

        try{
            pci_dev_str = buildPCIDeviceString(buff);
        }
        catch(CAILException &ex){
            std::cout << "CAILException: " << ex.getExceptionText() << std::endl;
        }

        return pci_dev_str;
    }

    std::string PCIInfo::buildPCIDeviceString(DataBuffer &buff){

        std::ostringstream pci_str_builder;

        unsigned short   vend_id       	    = getSpecificWidthOctets(buff, TWOBYTE, m_VENDOR_ID_POS),
                         subvend_id    	    = getSpecificWidthOctets(buff, TWOBYTE, m_SUB_VENDOR_ID_POS),
                         dev_id        	    = getSpecificWidthOctets(buff, TWOBYTE, m_DEVICE_ID_POS),
                         subdev_id     	    = getSpecificWidthOctets(buff, TWOBYTE, m_SUB_DEVICE_ID_POS),
                         class_code_id  	= getSpecificWidthOctets(buff, ONEBYTE, m_CLASS_CODE_POS),
                         subclass_code_id 	= getSpecificWidthOctets(buff, ONEBYTE, m_SUBCLASS_CODE_POS),
                         progif_code_id 	= getSpecificWidthOctets(buff, ONEBYTE, m_PROGIF_CODE_POS),
                         revision_id 		= getSpecificWidthOctets(buff, ONEBYTE, m_REVISION_ID_POS);

        std::string vend_id_str, device_id_str, class_code_id_str;
        unsigned int vend_id_loc, device_id_loc;

        if(class_code_id){
            class_code_id_str = getPCIClassString(class_code_id, subclass_code_id, progif_code_id);
        }

        vend_id_loc = getPCIIdentifierString(vend_id, vend_id_str);
        if( (vend_id_loc) ){
            device_id_loc = getPCIIdentifierString(dev_id, device_id_str, vend_id_loc, DEVICE);
        }

        /* Build partial PCI Device Identity string. */
        pci_str_builder << class_code_id_str << ": " << vend_id_str << " " << device_id_str << " (Revision " << std::hex << revision_id << std::dec << ")";

        return pci_str_builder.str();
    }

    /* Get XX bits from the PCI device address space */
    unsigned int PCIInfo::getSpecificWidthOctets(DataBuffer &buff, unsigned int width, unsigned int pos){

        unsigned char *buffer = buff.getBuffer();
        unsigned int result = 0;

        for(unsigned int i = 0; i < width; i++){
            result |= (buffer[pos+i] << (i*8));
        }

        return result;
    }

    unsigned int PCIInfo::getPCIIdentifierString(unsigned short id, std::string &id_str, unsigned int previous_loc, char extra_skip){

        char id_code_str[5];
        int res = 0;
        size_t id_str_loc = 0, end_newline_loc = 0;

        /* XXXX__<String> */
        short skip_distance = (6+extra_skip);

        res = snprintf(id_code_str, (sizeof(id_code_str)), "%x", id);

        std::string id_code_str_full("\n");
        if( (extra_skip) ){
            id_code_str_full += "\t";
            if( (extra_skip)==2 ){
                id_code_str_full += "\t";
            }
        }

        if( (res != (sizeof(id_code_str)-1)) ){
            for(unsigned int counter = 0; counter < ((sizeof(id_code_str)-1)-res); counter++){
                id_code_str_full += '0';
            }
        }
        id_code_str_full += id_code_str;

        id_str_loc = m_pci_id_file_data.find(id_code_str_full, previous_loc);
        if( (id_str_loc!=std::string::npos) ){
            /* Skip newline */
            id_str_loc++;
            end_newline_loc = m_pci_id_file_data.find("\n", id_str_loc);
            if( (end_newline_loc!=std::string::npos) ){
                id_str = m_pci_id_file_data.substr((id_str_loc+skip_distance), (end_newline_loc-id_str_loc-skip_distance));
            }
        }

        return id_str_loc;
    }

    std::string PCIInfo::getPCIClassString(unsigned short class_code, unsigned short subclass_code, unsigned short progif_code){

        std::string class_str = "No Class Defined";

        switch(class_code){
            case(0x0):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Any device except for VGA-Compatible devices";
                        break;
                    }
                    case(0x1):{
                        class_str = "VGA-Compatible Device";
                        break;
                    }
                }
                break;
            }
            case(0x1):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "SCSI Bus Controller";
                        break;
                    }
                    case(0x1):{
                        class_str = "IDE Controller";
                        break;
                    }
                    case(0x2):{
                        class_str = "Floppy Disk Controller";
                        break;
                    }
                    case(0x3):{
                        class_str = "IPI Bus Controller";
                        break;
                    }
                    case(0x4):{
                        class_str = "RAID Controller";
                        break;
                    }
                    case(0x5):{
                        switch(progif_code){
                            case(0x20):{
                                class_str = "ATA Controller (Single DMA)";
                                break;
                            }
                            case(0x30):{
                                class_str = "ATA Controller (Chained DMA)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x6):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Serial ATA (Vendor Specific Interface)";
                                break;
                            }
                            case(0x1):{
                                class_str = "Serial ATA (AHCI 1.0)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x7):{
                        class_str = "Serial Attached SCSI (SAS)";
                        break;
                    }
                    case(0x80):{
                        class_str = "Mass Storage Controller";
                        break;
                    }
                }
                break;
            }
            case(0x2):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Ethernet Controller";
                        break;
                    }
                    case(0x1):{
                        class_str = "Token Ring Controller";
                        break;
                    }
                    case(0x2):{
                        class_str = "FDDI Controller";
                        break;
                    }
                    case(0x3):{
                        class_str = "ATM Controller";
                        break;
                    }
                    case(0x4):{
                        class_str = "ISDN Controller";
                        break;
                    }
                    case(0x5):{
                        class_str = "WorldFip Controller";
                        break;
                    }
                    case(0x6):{
                        class_str = "PICMG 2.14 Multi Computing";
                        break;
                    }
                    case(0x80):{
                        class_str = "Network Controller";
                        break;
                    }
                }
                break;
            }
            case(0x3):{
                switch(subclass_code){
                    case(0x0):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "VGA-Compatible Controller";
                                break;
                            }
                            case(0x1):{
                                class_str = "8512-Compatible Controller";
                                break;
                            }
                            break;
                        }
                    }
                    case(0x1):{
                        class_str = "XGA Controller";
                        break;
                    }
                    case(0x2):{
                        class_str = "3D Controller (Not VGA-Compatible)";
                        break;
                    }
                    case(0x80):{
                        class_str = "Display Controller";
                        break;
                    }
                }
                break;
            }
            case(0x4):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Video Device";
                        break;
                    }
                    case(0x1):{
                        class_str = "Multimedia Audio Controller";
                        break;
                    }
                    case(0x2):{
                        class_str = "Computer Telephony Device";
                        break;
                    }
                    case(0x3):{
                        class_str = "Audio Device";
                        break;
                    }
                    case(0x80):{
                        class_str = "Multimedia Device";
                        break;
                    }
                }
                break;
            }
            case(0x5):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "RAM Controller";
                        break;
                    }
                    case(0x1):{
                        class_str = "Flash Controller";
                        break;
                    }
                    case(0x80):{
                        class_str = "Memory Controller";
                        break;
                    }
                }
                break;
            }
            case(0x6):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Host Bridge";
                        break;
                    }
                    case(0x1):{
                        class_str = "ISA Bridge";
                        break;
                    }
                    case(0x2):{
                        class_str = "EISA Bridge";
                        break;
                    }
                    case(0x3):{
                        class_str = "MCA Bridge";
                        break;
                    }
                    case(0x4):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "PCI-to-PCI Bridge";
                                break;
                            }
                            case(0x1):{
                                class_str = "PCI-to-PCI Bridge (Subtractive Decode)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x5):{
                        class_str = "PCMCIA Bridge";
                        break;
                    }
                    case(0x6):{
                        class_str = "NuBus Bridge";
                        break;
                    }
                    case(0x7):{
                        class_str = "CardBus Bridge";
                        break;
                    }
                    case(0x8):{
                        class_str = "RACEway Bridge";
                        break;
                    }
                    case(0x9):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "PCI-to-PCI Bridge (Semi-Transparent, Primary)";
                                break;
                            }
                            case(0x1):{
                                class_str = "PCI-to-PCI Bridge (Semi-Transparent, Secondary)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0xa):{
                        class_str = "InfiniBrand-to-PCI Host Bridge";
                        break;
                    }
                    case(0x80):{
                        class_str = "Bridge Device";
                        break;
                    }
                }
                break;
            }
            case(0x7):{
                switch(subclass_code){
                    case(0x0):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Generic XT-Compatible Serial Controller";
                                break;
                            }
                            case(0x1):{
                                class_str = "16450-Compatible Serial Controller";
                                break;
                            }
                            case(0x2):{
                                class_str = "16550-Compatible Serial Controller";
                                break;
                            }
                            case(0x3):{
                                class_str = "16650-Compatible Serial Controller";
                                break;
                            }
                            case(0x4):{
                                class_str = "16750-Compatible Serial Controller";
                                break;
                            }
                            case(0x5):{
                                class_str = "16850-Compatible Serial Controller";
                                break;
                            }
                            case(0x6):{
                                class_str = "16950-Compatible Serial Controller";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x1):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Parallel Port";
                                break;
                            }
                            case(0x1):{
                                class_str = "Bi-Directional Parallel Port";
                                break;
                            }
                            case(0x2):{
                                class_str = "ECP 1.X Compliant Parallel Port";
                                break;
                            }
                            case(0x3):{
                                class_str = "IEEE 1284 Controller";
                                break;
                            }
                            case(0xfe):{
                                class_str = "IEEE 1284 Target Device";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x2):{
                        class_str = "Multiport Serial Controller";
                        break;
                    }
                    case(0x3):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Generic Modem";
                                break;
                            }
                            case(0x1):{
                                class_str = "Hayes Compatible Modem (16450-Compatible Interface)";
                                break;
                            }
                            case(0x2):{
                                class_str = "Hayes Compatible Modem (16550-Compatible Interface)";
                                break;
                            }
                            case(0x3):{
                                class_str = "Hayes Compatible Modem (16650-Compatible Interface)";
                                break;
                            }
                            case(0x4):{
                                class_str = "Hayes Compatible Modem (16750-Compatible Interface)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x4):{
                        class_str = "IEEE 488.1/2 (GPIB) Controller";
                        break;
                    }
                    case(0x5):{
                        class_str = "Smart Card";
                        break;
                    }
                    case(0x80):{
                        class_str = "Communications Device";
                        break;
                    }
                }
                break;
            }
            case(0x8):{
                switch(subclass_code){
                    case(0x0):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Generic 8259 PIC";
                                break;
                            }
                            case(0x1):{
                                class_str = "ISA PIC";
                                break;
                            }
                            case(0x2):{
                                class_str = "EISA PIC";
                                break;
                            }
                            case(0x10):{
                                class_str = "I/O APIC Interrupt Controller";
                                break;
                            }
                            case(0x20):{
                                class_str = "I/O(x) APIC Interrupt Controller";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x1):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Generic 8237 DMA Controller";
                                break;
                            }
                            case(0x1):{
                                class_str = "ISA DMA Controller";
                                break;
                            }
                            case(0x2):{
                                class_str = "EISA DMA Controller";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x2):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Generic 8254 System Timer";
                                break;
                            }
                            case(0x1):{
                                class_str = "ISA System Timer";
                                break;
                            }
                            case(0x2):{
                                class_str = "EISA System Timer";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x3):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Generic RTC Controller";
                                break;
                            }
                            case(0x1):{
                                class_str = "ISA RTC Controller";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x4):{
                        class_str = "Generic PCI Hot-Plug Controller";
                        break;
                    }
                    case(0x80):{
                        class_str = "System Peripheral";
                        break;
                    }
                }
                break;
            }
            case(0x9):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Keyboard Controller";
                        break;
                    }
                    case(0x1):{
                        class_str = "Digitizer";
                        break;
                    }
                    case(0x2):{
                        class_str = "Mouse Controller";
                        break;
                    }
                    case(0x3):{
                        class_str = "Scanner Controller";
                        break;
                    }
                    case(0x4):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "Gameport Controller (Generic)";
                                break;
                            }
                            case(0x10):{
                                class_str = "Gameport Contrlller (Legacy)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x80):{
                        class_str = "Input Controller";
                        break;
                    }
                }
                break;
            }
            case(0xa):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Generic Docking Station";
                        break;
                    }
                    case(0x80):{
                        class_str = "Docking Station";
                        break;
                    }
                }
                break;
            }
            case(0xb):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "386 Processor";
                        break;
                    }
                    case(0x1):{
                        class_str = "486 Processor";
                        break;
                    }
                    case(0x2):{
                        class_str = "Pentium Processor";
                        break;
                    }
                    case(0x10):{
                        class_str = "Alpha Processor";
                        break;
                    }
                    case(0x20):{
                        class_str = "PowerPC Processor";
                        break;
                    }
                    case(0x30):{
                        class_str = "MIPS Processor";
                        break;
                    }
                    case(0x40):{
                        class_str = "Co-Processor";
                        break;
                    }
                }
                break;
            }
            case(0xc):{
                switch(subclass_code){
                    case(0x0):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "IEEE 1394 Controller (FireWire)";
                                break;
                            }
                            case(0x10):{
                                class_str = "IEEE 1394 Controller (1394 OpenHCI Spec)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x1):{
                        class_str = "ACCESS.bus";
                        break;
                    }
                    case(0x2):{
                        class_str = "SSA";
                        break;
                    }
                    case(0x3):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "USB (Universal Host Controller Spec)";
                                break;
                            }
                            case(0x10):{
                                class_str = "USB (Open Host Controller Spec)";
                                break;
                            }
                            case(0x20):{
                                class_str = "USB2 Host Controller (Intel Enhanced Host Controller Interface)";
                                break;
                            }
                            case(0x80):{
                                class_str = "USB";
                                break;
                            }
                            case(0xfe):{
                                class_str = "USB (Not Host Controller)";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x4):{
                        class_str = "Fibre Channel";
                        break;
                    }
                    case(0x5):{
                        class_str = "SMBus";
                        break;
                    }
                    case(0x6):{
                        class_str = "InfiniBand";
                        break;
                    }
                    case(0x7):{
                        switch(progif_code){
                            case(0x0):{
                                class_str = "IPMI SMIC Interface";
                                break;
                            }
                            case(0x1):{
                                class_str = "IPMI Kybd Controller Style Interface";
                                break;
                            }
                            case(0x2):{
                                class_str = "IPMI Block Transfer Interface";
                                break;
                            }
                        }
                        break;
                    }
                    case(0x8):{
                        class_str = "SERCOS Interface Standard (IEC 61491)";
                        break;
                    }
                    case(0x9):{
                        class_str = "CANbus";
                        break;
                    }
                }
                break;
            }
            case(0xd):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "iRDA Compatible Controller";
                        break;
                    }
                    case(0x1):{
                        class_str = "Consumer IR Controller";
                        break;
                    }
                    case(0x10):{
                        class_str = "RF Controller";
                        break;
                    }
                    case(0x11):{
                        class_str = "Bluetooth Controller";
                        break;
                    }
                    case(0x12):{
                        class_str = "Broadband Controller";
                        break;
                    }
                    case(0x20):{
                        class_str = "Ethernet Controller (802.11a)";
                        break;
                    }
                    case(0x21):{
                        class_str = "Ethernet Controller (802.11b)";
                        break;
                    }
                    case(0x80):{
                        class_str = "Wireless Controller";
                        break;
                    }
                }
                break;
            }
            case(0xe):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Message FIFO";
                        break;
                    }
                    default:{
                        class_str = "I20 Architecture";
                        break;
                    }
                }
                break;
            }
            case(0xf):{
                switch(subclass_code){
                    case(0x1):{
                        class_str = "TV Controller";
                        break;
                    }
                    case(0x2):{
                        class_str = "Audio Controller";
                        break;
                    }
                    case(0x3):{
                        class_str = "Voice Controller";
                        break;
                    }
                    case(0x4):{
                        class_str = "Data Controller";
                        break;
                    }
                }
                break;
            }
            case(0x10):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "Network and Computing Encrpytion/Decryption";
                        break;
                    }
                    case(0x10):{
                        class_str = "Entertainment Encryption/Decryption";
                        break;
                    }
                    case(0x80):{
                        class_str = "Encryption/Decryption";
                        break;
                    }
                }
                break;
            }
            case(0x11):{
                switch(subclass_code){
                    case(0x0):{
                        class_str = "DPIO Modules";
                        break;
                    }
                    case(0x1):{
                        class_str = "Performance Counters";
                        break;
                    }
                    case(0x10):{
                        class_str = "Communications Syncrhonization Plus Time and Frequency Test/Measurment";
                        break;
                    }
                    case(0x20):{
                        class_str = "Management Card";
                        break;
                    }
                    case(0x80):{
                        class_str = "Data Acquisition/Signal Processing Controller";
                        break;
                    }
                }
                break;
            }
            default:{
                class_str = "Device does not fit any defined class";
                break;
            }
        }

        return class_str;
    }

}
/***********************************************************************************************************************/
