
#ifndef	EPCS_H_
#define	EPCS_H_

#include <stdio.h>
#include "linux/types.h"
#include "fpga.h"

#define epcs_read    0x03
#define epcs_pp      0x02
#define epcs_wren    0x06
#define epcs_wrdi    0x04
#define epcs_rdsr    0x05
#define epcs_wrsr    0x01
#define epcs_se      0xD8
#define epcs_be      0xC7

#define epcs_dp      0xB9
#define epcs_res     0xAB

//#define BASE_ADDR	(0xa0020000+0x100*4)

//#define FPGA_EPCS_BASE 0x800

//#define EPCS16_SECTOR_SIZE	 0x40000
//#define EPCS16_BLOCK_SIZE	 (EPCS16_SECTOR_SIZE * 32)

#define FPGA_EPCS_BASE  0x400
#define JIC_INVALID_BYTES	 173
#define EPCS64_SECTOR_SIZE	 0x10000
#define EPCS64_BLOCK_SIZE	 (EPCS64_SECTOR_SIZE * 128)



#define ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N			  				(0x02)
#define ALTERA_AVALON_SPI_STATUS_TRDY_MSK             (0x40)
#define ALTERA_AVALON_SPI_STATUS_RRDY_MSK             (0x80)
#define ALTERA_AVALON_SPI_STATUS_TMT_MSK              (0x20)
#define ALT_AVALON_SPI_COMMAND_MERGE				  							(0x01)
#define ALTERA_AVALON_SPI_CONTROL_SSO_MSK             (0x400)

#define IORD_ALTERA_AVALON_SPI_RXDATA(base)           FPGA_READ16(base, 0x200)
#define IORD_ALTERA_AVALON_SPI_STATUS(base)           FPGA_READ16(base, 0x204)

#define IOWR_ALTERA_AVALON_SPI_TXDATA(base, data)     FPGA_WRITE16(base, 0x202, data);
#define IOWR_ALTERA_AVALON_SPI_CONTROL(base, data)    FPGA_WRITE16(base, 0x206, data);
#define IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, data)  FPGA_WRITE16(base, 0x20a, data);
#define ALT_MAX_NUMBER_OF_FLASH_REGIONS     8


/*
 * Description of a single Erase region
 */
typedef struct flash_region
{
  int   offset;
  int   region_size;
  int   number_of_blocks;
  int   block_size;
}flash_region;


typedef struct alt_flash_dev alt_flash_dev;
typedef alt_flash_dev alt_flash_fd;

struct alt_flash_dev
{
  struct  alt_flash_dev*    llist;//alt_llist
  const char*               name;
  int*						open;
  int*						close;
  int*						write;
  int*						read;
  int*						get_info;
  int*						erase_block;
  int*						write_block;

  void*                     base_addr;
  int                       length;
  int                       number_of_regions;
  flash_region              region_info[ALT_MAX_NUMBER_OF_FLASH_REGIONS]; //flash_region
};



typedef struct alt_flash_epcs_dev alt_flash_epcs_dev;

/*
 *  Description of the flash
 *
 *  Contains the basic alt_flash_dev, plus
 *  epcs-specific parameters.
 *
 *  Every parameter that distinguishes this
 *  serial flash device from any other is
 *  encoded here.
 *
 *  Example:
 *  size_in_bytes: the number of bytes of
 *  storage in this chip.
 */
struct alt_flash_epcs_dev
{
  alt_flash_dev dev;

  __u32 register_base;
  __u32 size_in_bytes;
  __u32 silicon_id;
  __u32 page_size;
};

/* This is a very simple routine which performs one SPI master transaction.
 * It would be possible to implement a more efficient version using interrupts
 * and sleeping threads but this is probably not worthwhile initially.
 */

int alt_avalon_spi_command(__u32 base, __u32 slave,
						   __u32 write_length, const __u8 * write_data,
						   __u32 read_length, __u8 * read_data,
						   __u32 flags);

/* This might be a candidate for optimization.  Precompute the last-address? */
static  int alt_epcs_test_address(alt_flash_dev* flash_info, int offset);//ɾ�������� ALT_INLINE

void epcs_write_enable(__u32 base);

void epcs_sector_erase(__u32 base, __u32 offset);
void epcs_bulk_erase(__u32 base);

__u8 epcs_read_status_register(__u32 base);

static  int epcs_test_wip(__u32 base);//ɾ�������� ALT_INLINE

static  void epcs_await_wip_released(__u32 base);//ɾ�������� ALT_INLINE

__u32 epcs_write_buffer(__u32 base, int offset, const __u8* src_addr, int length);

__u32 epcs_read_buffer(__u32 base, int offset, __u8 *dest_addr, int length);

/*
*
* Erase the selected erase block ("sector erase", from the POV
* of the EPCS data sheet).
*/
int alt_epcs_flash_erase_block(alt_flash_dev* flash_info, int block_offset);

/* Write, assuming that someone has kindly erased the appropriate
* sector(s).
* Note: "block_offset" is the base of the current erase block.
* "data_offset" is the absolute address (from the 0-base of this
* device's memory) of the beginning of the write-destination.
* This device has no need for "block_offset", but it's included for
* function type compatibility.
*/
int alt_epcs_flash_write_block(alt_flash_dev* flash_info, int block_offset,
							   int data_offset, const void* data,
							   int length);
/*
*  If you try to read beyond the end of the memory, you'll wrap around
*  to the beginning.  Reads that start beyond the end of the memory are
*  flagged as errors with EIO (is there a better error code?).
*/
int alt_epcs_flash_read(alt_flash_dev* flash_info, int offset,
						void* dest_addr, int length);

__u8 epcs_read_electronic_signature(__u32 base);
static void epcs_await_wip_released(__u32 base);

void init_alt_flash_epcs_dev(alt_flash_epcs_dev *f);

#endif
