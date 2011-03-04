#include <asm/arch/hardware.h>


#define MCIPOWER       (IO_ADDRESS_MCI_BASE +0x0)
#define MCICLOCK       (IO_ADDRESS_MCI_BASE +0x4)
#define MCIARGUMENT    (IO_ADDRESS_MCI_BASE +0x8)
#define MCICOMMAND     (IO_ADDRESS_MCI_BASE +0xc)
#define MCIRESPCMD     (IO_ADDRESS_MCI_BASE +0x10)
#define MCIRESPONSE0   (IO_ADDRESS_MCI_BASE +0x14)
#define MCIRESPONSE1   (IO_ADDRESS_MCI_BASE +0x18)
#define MCIRESPONSE2   (IO_ADDRESS_MCI_BASE +0x1c)
#define MCIRESPONSE3   (IO_ADDRESS_MCI_BASE +0x20)
#define MCIDATATIMER   (IO_ADDRESS_MCI_BASE +0x24)
#define MCIDATALENGTH  (IO_ADDRESS_MCI_BASE +0x28)
#define MCIDATACTRL    (IO_ADDRESS_MCI_BASE +0x2c)
#define MCIDATACNT     (IO_ADDRESS_MCI_BASE +0x30)
#define MCISTATUS      (IO_ADDRESS_MCI_BASE +0x34)
#define MCICLEAR       (IO_ADDRESS_MCI_BASE +0x38)
#define MCIMASK0       (IO_ADDRESS_MCI_BASE +0x3c)
#define MCIMASK1       (IO_ADDRESS_MCI_BASE +0x40)
#define MCISELECT      (IO_ADDRESS_MCI_BASE +0x44)
#define MCIFIFOCNT     (IO_ADDRESS_MCI_BASE +0x48)
#define MCIFIFO        (IO_ADDRESS_MCI_BASE +0x80)
#define MCIPERIPHID0   (IO_ADDRESS_MCI_BASE +0xfe0)
#define MCIPERIPHID1   (IO_ADDRESS_MCI_BASE +0xfe4)
#define MCIPERIPHID2   (IO_ADDRESS_MCI_BASE +0xfe8)
#define MCIPERIPHID3   (IO_ADDRESS_MCI_BASE +0xfec)
#define MCIPCELLID0    (IO_ADDRESS_MCI_BASE +0xff0)
#define MCIPCELLID1    (IO_ADDRESS_MCI_BASE +0xff4)
#define MCIPCELLID2    (IO_ADDRESS_MCI_BASE +0xff8)
#define MCIPCELLID3    (IO_ADDRESS_MCI_BASE +0xffc)  

#define MCI_STATUS_CmdCrcFail_Mask 0x1
#define MCI_STATUS_DataCrcFail_Mask 0x2
#define MCI_STATUS_CmdTimeOut_Mask 0x4
#define MCI_STATUS_DataTimeOut_Mask 0x8
#define MCI_STATUS_CmdRespEnd_Mask 0x40
#define MCI_STATUS_CmdSent_Mask 0x80
#define MCI_STATUS_DataEnd_Mask 0x100
#define MCI_STATUS_DataBlockEnd_Mask 0x400
#define MCI_STATUS_CmdActive_Mask 0x800

#define MCI_BusWudth_4bits 4
#define MCI_BusWidth_1bit  1
#define MCI_BusWudth_Mask  0x800

#define MCI_Bypass_ON 1
#define MCI_Bypass_OFF 0
#define MCI_Bypass_Mask  0x400

#define MCI_Enable_ON 1
#define MCI_Enable_OFF 0
#define MCI_Enable_Mask  0x100

#define MCI_PwrSave_ON 1
#define MCI_PwrSave_OFF  0
#define MCI_PwrSave_Mask 0x200

#define MCI_Clock_Mask  0xff

#define MCI_PwrOn 0x3
#define MCI_PwrUp 0x2
#define MCI_PwrOff 0

#define PCR_MCI_PCLK                IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x15c
#define PCR_MCI_MCLK                IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x160
#define PSR_MCI_PCLK                IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x30c
#define PSR_MCI_MCLK                IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x310

#define SCR_CLK_MCI                 IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x20
#define FS1_CLK_MCI                 IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x6c
#define FS2_CLK_MCI                 IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0xb8
#define SSR_CLK_MCI                 IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x104
#define ESR_MCI_PCLK                IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x4bc
#define ESR_MCI_MCLK                IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x4c0

#define PCR_IOCONF_PCLK             IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x20c
#define PSR_IOCONF_PCLK             IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x3bc