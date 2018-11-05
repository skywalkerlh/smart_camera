
#define  _EPCS16_C_

#include "epcs.h"

#include <stdio.h>
#include <unistd.h>


int alt_avalon_spi_command(__u32 base, __u32 slave,
																		__u32 write_length, const __u8 * write_data,
																		__u32 read_length, __u8 * read_data,
																		__u32 flags)
{
	const __u8 * write_end = write_data + write_length;
	__u8 * read_end = read_data + read_length;

	__u32 write_zeros = read_length;
	__u32 read_ignore = write_length;
	__u32 status = 0;

	__u32 tmp;

	/* We must not send more than two bytes to the target before it has
	* returned any as otherwise it will overflow. */
	/* Unfortunately the hardware does not seem to work with credits > 1,
	* leave it at 1 for now. */
	volatile int credits = 1;

	/* Warning: this function is not currently safe if called in a multi-threaded
	* environment, something above must perform locking to make it safe if more
	* than one thread intends to use it.
	*/

	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, 1 << slave);

	/* Set the SSO bit (force chipselect) only if the toggle flag is not set */
	if ((flags & ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N) == 0x02) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	}

	/*
	* Discard any stale data present in the RXDATA register, in case
	* previous communication was interrupted and stale data was left
	* behind.
	*/
	status = IORD_ALTERA_AVALON_SPI_RXDATA(base);

	/* Keep clocking until all the data has been processed. */
	for ( ; ; )
	{

		do
		{
			status = IORD_ALTERA_AVALON_SPI_STATUS(base);
			/*
			if(status&0x100)
			{
				status &= 0xFFFFFFFF;
			}

			if(((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0 || credits == 0))
			{
				status &= 0xFFFFFFFF;
			}
			*/
		}
	 while (((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0 || credits == 0) &&
	         (status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) == 0);

		if ((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) != 0 && credits > 0)//0x40
		{
			credits--;

			if (write_data < write_end)
			{
				IOWR_ALTERA_AVALON_SPI_TXDATA(base, *write_data++);
			}
			else if (write_zeros > 0)
			{
				write_zeros--;
				IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0);
			}
			else
				credits = -1024;
		}


		if ((status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) != 0)//0x80
		{
			__u32 rxdata = IORD_ALTERA_AVALON_SPI_RXDATA(base);

			if (read_ignore > 0)
				read_ignore--;
			else
				*read_data++ = (__u8)rxdata;
			credits++;

			if (read_ignore == 0 && read_data == read_end)
				break;
		}

	}

	/* Wait until the interface has finished transmitting */
	do
	{
		status = IORD_ALTERA_AVALON_SPI_STATUS(base);
	}
	while ((status & ALTERA_AVALON_SPI_STATUS_TMT_MSK) != 0x20);//0x20

	/* Clear SSO (release chipselect) unless the caller is going to
	* keep using this chip
	*/
	if ((flags & ALT_AVALON_SPI_COMMAND_MERGE) == 0x01)//0x01
		IOWR_ALTERA_AVALON_SPI_CONTROL(base, 0);

	return read_length;

}

//-----------------------------------------------------------------------------------

/* This might be a candidate for optimization.  Precompute the last-address? */
static inline int alt_epcs_test_address(alt_flash_dev* flash_info, int offset)
{
	int ret_code = 0;
	/* Error checking:
	* if the block offset is outside of the memory, return -EIO.
	*/
	alt_flash_epcs_dev *f = (alt_flash_epcs_dev*)flash_info;

	const __u32 last_region_index = f->dev.number_of_regions - 1;
	__u32 last_device_address =
		-1 +
		f->dev.region_info[last_region_index].offset +
		f->dev.region_info[last_region_index].region_size;

	if (offset > last_device_address)
	{
		/* Someone tried to erase a block outside of this device's range. */
		ret_code = -1;//EIO
	}
	return ret_code;
}
void epcs_write_enable(__u32 base)
{
	const __u8 wren = epcs_wren;
	alt_avalon_spi_command(
		base,
		0,
		1,
		&wren,
		0,
		(__u8*)0,
		0
		);
}



void epcs_sector_erase(__u32 base, __u32 offset)
{
	__u8 se[4];

	se[0] = epcs_se;
	se[1] = (offset >> 16) & 0xFF;
	se[2] = (offset >> 8) & 0xFF;
	se[3] = offset & 0xFF;

	epcs_write_enable(base);

	alt_avalon_spi_command(
		base,
		0,
		sizeof(se) / sizeof(*se),
		se,
		0,
		(__u8*)0,
		0
		);

	epcs_await_wip_released(base);
}
//----------------------------------------------------
__u8 epcs_read_status_register(__u32 base)
{
	const __u8 rdsr = epcs_rdsr;
	__u8 status = 0;
	
	alt_avalon_spi_command(
		base,
		0,
		1,
		&rdsr,
		1,
		&status,
		ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N|ALT_AVALON_SPI_COMMAND_MERGE
		);
	
	return status;
}
static int epcs_test_wip(__u32 base)//ɾ�������� ALT_INLINE
{
	__u8 status;
	status = epcs_read_status_register(base);
	if(status&0x01)
	{
		return 1;
	}
	else
	{
		return 0;
	}
//	return status & 1;
}

static void epcs_await_wip_released(__u32 base)
{
	/* Wait until the WIP bit goes low. */
	while (epcs_test_wip(base))
	{
		//usleep(10000);
		;
	}
}
//-------------------------------------------------------------
/* Write a partial or full page, assuming that page has been erased */
__u32 epcs_write_buffer(__u32 base, int offset, const __u8* src_addr, int length)
{
	__u8 pp[4];

	pp[0] = epcs_pp;
	pp[1] = (offset >> 16) & 0xFF;
	pp[2] = (offset >> 8) & 0xFF;
	pp[3] = offset & 0xFF;

	/* First, WREN */
	epcs_write_enable(base);
	usleep(5);
	/* Send the PP command */
	alt_avalon_spi_command(
		base,
		0,
		sizeof(pp) / sizeof(*pp),
		pp,
		0,
		(__u8*)0,
		ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N
		);

	/* Send the user's buffer */
	alt_avalon_spi_command(
		base,
		0,
		length,
		src_addr,
		0,
		(__u8*)0,
		ALT_AVALON_SPI_COMMAND_MERGE
		);
	usleep(5);
	epcs_await_wip_released(base);
	/* Wait until the write is done.  This could be optimized -
	* if the user's going to go off and ignore the flash for
	* a while, its writes could occur in parallel with user code
	* execution.  Unfortunately, I have to guard all reads/writes
	* with wip-tests, to make that happen.
	*/
//	epcs_await_wip_released(base);
//	printf("%d\n",sum++);
	return length;
}
//---------------------------------------------------------------------
__u32 epcs_read_buffer(__u32 base, int offset, __u8 *dest_addr, int length)
{
	__u8 read_command[4];

	read_command[0] = epcs_read;
	read_command[1] = (offset >> 16) & 0xFF;
	read_command[2] = (offset >> 8) & 0xFF;
	read_command[3] = offset & 0xFF;

#if 0
	/* If a write is in progress, fail. */
	if (epcs_test_wip(base))
		return 0;
#endif
	/* I don't know why this is necessary, since I call await-wip after
	* all writing commands.
	*/
	epcs_await_wip_released(base);

	alt_avalon_spi_command(
		base,
		0,
		sizeof(read_command) / sizeof(*read_command),
		read_command,
		length,
		(__u8*)dest_addr,
		0
		);

	return length;
}
//----------------------------------------------------------------------------
/*
*
* Erase the selected erase block ("sector erase", from the POV
* of the EPCS data sheet).
*/
int alt_epcs_flash_erase_block(alt_flash_dev* flash_info, int block_offset)
{
	int ret_code = 0;
	alt_flash_epcs_dev *f = (alt_flash_epcs_dev*)flash_info;

	init_alt_flash_epcs_dev(f);

	ret_code = alt_epcs_test_address(flash_info, block_offset);

	if (ret_code >= 0)
	{

   		 /* Execute a WREN instruction */
   	 	epcs_write_enable(f->register_base);

		/* Send the Sector Erase command, whose 3 address bytes are anywhere
		* within the chosen sector.
		*/
		epcs_sector_erase(f->register_base, block_offset);
	}
	return ret_code;
}

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
							   int length)
{
	int ret_code;
	alt_flash_epcs_dev *f = (alt_flash_epcs_dev*)flash_info;
	int dalay = 0;

	int buffer_offset = 0;
	int length_of_current_write;
	
	init_alt_flash_epcs_dev(f);

	ret_code = alt_epcs_test_address(flash_info, data_offset);

	if (ret_code >= 0)
	{

		/* "Block" writes must be broken up into the page writes that
		* the device understands.  Partial page writes are allowed.
		*/
		while (length)
		{
			int next_page_start = (data_offset + f->page_size) & ~(f->page_size - 1);
			//length_of_current_write = MIN(length, next_page_start - data_offset);
			length_of_current_write = length>=(next_page_start - data_offset)?(next_page_start - data_offset):length;

			epcs_write_buffer(f->register_base, data_offset, &((const __u8*)data)[buffer_offset], length_of_current_write);

			length -= length_of_current_write;
			buffer_offset += length_of_current_write;
			data_offset = next_page_start;

			usleep(5);
			//for(dalay =0;dalay<300;dalay++);
		}
	}

	return ret_code;
}

/*
*  If you try to read beyond the end of the memory, you'll wrap around
*  to the beginning.  Reads that start beyond the end of the memory are
*  flagged as errors with EIO (is there a better error code?).
*/
int alt_epcs_flash_read(alt_flash_dev* flash_info, int offset,
						void* dest_addr, int length)
{
	int ret_code = 0;

	alt_flash_epcs_dev *f = (alt_flash_epcs_dev*)flash_info;

	init_alt_flash_epcs_dev(f);

	ret_code = alt_epcs_test_address(flash_info, offset);

	if (ret_code >= 0)
	{
		ret_code = epcs_read_buffer(f->register_base, offset, dest_addr, length);

		/* epcs_read_buffer returns the number of buffers read, but
		* alt_epcs_flash_read returns 0 on success, <0 on failure.
		*/
		if (ret_code == length)
		{
			ret_code = 0;
		}
	}
	return ret_code;
}

__u8 epcs_read_electronic_signature(__u32 base)
{
  const __u8 res_cmd[] = {epcs_res, 0, 0, 0};
  __u8 res;

  alt_avalon_spi_command(
    base,
    0,
    sizeof(res_cmd) / sizeof(*res_cmd),
    res_cmd,
    1,
    &res,
    0
  );

  return res;

}

void init_alt_flash_epcs_dev(alt_flash_epcs_dev *flash)
{
	flash->silicon_id = 0x16;
	flash->dev.region_info[0].region_size = 64 * 1024 * 1024 / 8;
	flash->dev.region_info[0].number_of_blocks = 128;
	flash->dev.region_info[0].block_size = 65536;
	flash->size_in_bytes = flash->dev.region_info[0].region_size;
	flash->dev.number_of_regions = 1;
	flash->dev.region_info[0].offset = 0;
	flash->page_size = 256;
	flash->register_base = (__u32)(flash->dev.base_addr);
}

void epcs_bulk_erase(__u32 base)
{
	__u8 cmd = epcs_be;

	epcs_write_enable(base);
	//usleep(50);
	alt_avalon_spi_command(
		base,
		0,
		1,
		&cmd,
		0,
		(__u8*)0,
		0
		);

	//usleep(50);

	epcs_await_wip_released(base);
}




