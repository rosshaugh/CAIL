/*
 * CAILPCIInfo.h
 *
 *  Created on: Aug 23, 2013
 *      Author: rhaugh
 */

/*
 * Reference: http://wiki.osdev.org/PCI
 *
 * register	| bits 31-24 | bits 23-16  |   bits 15-8   |    bits 7-0     |
 *   00     |        Device ID         |           Vendor ID             |
 *   04	    |          Status          |            Command              |
 *   08	    | Class code |  Subclass   |    Prog IF    |   Revision ID   |
 *   0C	    |    BIST    | Header type | Latency Timer | Cache Line Size |
 *   ...
 *   ...
 *   2C     |       Subsystem ID       |      Subsystem Vendor ID        |
 */

#ifndef CAILPCIINFO_H_
#define CAILPCIINFO_H_

#include <CAILGlobals.h>
#include <CAILException.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <vector>

namespace CAIL{

    class PCIInfo{

    public:
        PCIInfo();
        ~PCIInfo();
        void getPCIValues(std::vector<std::string> &);

        class DataBuffer{

        public:
            DataBuffer(size_t, const char *);
            DataBuffer(const char *);
            ~DataBuffer();

            unsigned char *getBuffer();
            const char *getFileName();
            size_t getSize();
            void populateBuffer();
            void initialiseBuffer();
        protected:
            DataBuffer();
        private:
            unsigned char *m_buffer;
            size_t m_size;
            std::string m_file_name;
            FILE *data_fp;
        };

        enum {
            ONEBYTE=1,
            TWOBYTE=2
        } WIDTH;

        enum {
            VENDOR,
            DEVICE,
            SUBVENDOR
        } IDTYPE;

    private:
        void initialise();
        void getPCIDirectories(std::vector<std::string> &);
        void getPCIIdenfifierFileContents();
        unsigned int getSpecificWidthOctets(DataBuffer &, unsigned int, unsigned int);

        std::string getPCIDeviceString(DataBuffer &);
        std::string buildPCIDeviceString(DataBuffer &);

        std::string getPCIClassString(unsigned short, unsigned short, unsigned short);
        unsigned int getPCIIdentifierString(unsigned short, std::string &, unsigned int = 0, char = 0);

        std::string m_pci_dir, m_pcix_dir, m_config_file, m_pci_id_file_loc, m_pci_id_file_data;

        /* pci.ids file contents */
        DataBuffer *pci_ids_data;

        unsigned short m_VENDOR_ID_POS,
                         m_SUB_VENDOR_ID_POS,
                         m_SUB_DEVICE_ID_POS,
                         m_DEVICE_ID_POS,
                         m_CLASS_CODE_POS,
                         m_SUBCLASS_CODE_POS,
                         m_PROGIF_CODE_POS,
                         m_REVISION_ID_POS,
                         m_CONFIG_AMOUNT;
    };

}

#endif /* CAILPCIINFO_H_ */
