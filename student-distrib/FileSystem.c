
#include "FileSystem.h"
#include "lib.h"
#include "SystemCall.h"


static uint32_t   n_dentry; // number of directory entries, should be 63
static uint32_t   n_inode;  // number of inode blocks (N)
static uint32_t   n_data;   // number of data blocks (D)
static dentry_t*  p_dentry; // start address for the entry part (p_dentry[0] for '.')
static inode_t*   p_inode;  // start address for the inode part 
static data_t*    p_data;   // start address for the data part

/* 
 * filesys_init
 *   DESCRIPTION: initialize parameters and pointers for file system
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pointers initialized
 */
void fsys_init(){
    n_dentry =  ((boot_block_t*)fsys_addr) -> num_dentry;
    n_inode  =  ((boot_block_t*)fsys_addr) -> num_inode;
    n_data   =  ((boot_block_t*)fsys_addr) -> num_data;
    p_dentry =  (dentry_t*)(((boot_block_t*)fsys_addr) -> dentries);
    p_inode  =  ((inode_t*)fsys_addr) + 1;
    p_data   =  ((data_t*)fsys_addr) + n_inode + 1;
}
/* 
 * strlen
 *   DESCRIPTION: count the lenghth of a string 
 *   INPUTS: string - a string ended by '\0'
 *   OUTPUTS: none
 *   RETURN VALUE: length of the string on success (0 to 32), -1 on fail (> MAX_NAME)
 *   SIDE EFFECTS: none
 */

uint32_t strlen_xjz(const uint8_t* string) {
    uint32_t len = 0;  
    while(string[len]!='\0'){
        len ++;
    }
    return len;
}
// NEED CHECK!!!!!!!!
/* 
 * read_dentry_by_name
 *   DESCRIPTION: read the directory entry that has the given name
 *   INPUTS: fname - string of file name
 *           dentry - structure to pass output
 *   OUTPUTS: dentry structure (filled)
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* f_name, dentry_t* dentry) {

    // sanity check: name length
    uint32_t f_name_len = strlen_xjz(f_name);
    if (f_name_len > MAX_NAME){
        return -1;
    }

    uint32_t i;
    uint32_t j;
    uint32_t signal;      // 1 for a successful match, 0 for not
    uint8_t* f_name_b;

    for (i = 0; i < n_dentry; ++i){
        f_name_b = p_dentry[i].f_name;
        signal = 1;
        // compare within the length
        for (j = 0; j < f_name_len; ++j){
            if (f_name_b[j] != f_name[j]){
              signal = 0;
              break;
            }
        }
        // check if f_name is shorter than target
        if((f_name_len != MAX_NAME) && (f_name_b[f_name_len]!= '\0')){
            signal = 0;
        }
        // match success, fill the dentry
        if(signal == 1){
            strncpy((int8_t*)dentry -> f_name, (int8_t*)f_name_b, f_name_len);
            if(f_name_len < MAX_NAME){
                dentry -> f_name[f_name_len] = '\0';
            }
            dentry -> f_type  = p_dentry[i].f_type;
            dentry -> inode   = p_dentry[i].inode;
            return 0;
        }
    }

    return -1;
}

/* 
 * read_dentry_by_index
 *   DESCRIPTION: read the directory entry that has given index
 *   INPUTS: index - index of the dentry
 *           dentry - structure to pass output
 *   OUTPUTS: dentry structure
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    // sanity check
    if (index >= n_dentry || index < 0) {
      return -1;
    }
    // count the length of name
    uint32_t f_name_len = 0;  
    // attention the cast HERE
    uint8_t* dentry_name = p_dentry[index].f_name;
    while(dentry_name[f_name_len]!='\0'){
        f_name_len ++;
        if(f_name_len == MAX_NAME){break;}    // end if reaches 32
    }
    // fill the dentry
    strncpy((int8_t*)dentry -> f_name, (int8_t*) dentry_name, f_name_len);
    if(f_name_len < MAX_NAME){
        dentry -> f_name[f_name_len] = '\0';
    }
    dentry -> f_type  = p_dentry[index].f_type;
    dentry -> inode   = p_dentry[index].inode;

    return 0;
}

/* 
 * read_data
 *   DESCRIPTION: read data in certain file
 *   INPUTS: inode - inode number of file
 *           offset - starting index (in byte) of reading
 *           buf - buffer that stores the data
 *           length - amount of bytes to read
 *   OUTPUTS: buf that contains data
 *   RETURN VALUE: number of bytes readed, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    // return value correct
    inode_t file       = p_inode[inode];
    // sanity check 
    if (buf == NULL)      {return -1;}
    if (inode >= n_inode) {return -1;}
    if (file.length <= offset) {return 0;}              // modified here
    
    uint32_t block_start  = offset/BLOCK_SIZE;
    uint32_t block_end    = (offset+length)/BLOCK_SIZE; // the last data
    uint32_t start_offset = offset%BLOCK_SIZE;
    uint32_t end_offset   = (offset+length)%BLOCK_SIZE; // data[end_offset] is the last offset
    uint32_t final_block  = file.length/BLOCK_SIZE; 
    uint32_t final_offset = file.length%BLOCK_SIZE;     // data[final_offset-1] is the last byte of this file
    if (final_offset == 0) {final_offset = BLOCK_SIZE;}
    // uint32_t index;
    uint32_t i,j;
    uint32_t end_point;                                 // inner-block-loop end sentry
    uint32_t start_point;                               // inner-block-loop start sentry
    data_t* current_block;                              // pointer to current data block

    uint32_t byte_cnt = 0;                              // count the byte written to buf

    for (i = block_start; i<= block_end; i++){
        // sanity check : validation of the data block index
        if(file.idx_data[i] >= n_data) {return -1;}

        current_block = p_data + file.idx_data[i];
        // set the start point
        start_point = (i == block_start) ? start_offset : 0;
        // set the end point
        if (i == final_block){
            if ( i == block_end ){
                end_point = (final_offset < end_offset) ? final_offset : end_offset;
            }else{
                end_point = final_offset;
            }   
        }else{
            end_point = (i == block_end) ? end_offset : BLOCK_SIZE ;
        }

        // write to the buf
        for(j = start_point; j< end_point; j++){
            buf[byte_cnt] = current_block->data[j];
            byte_cnt ++;
        }
        if ( (i == final_block) || (i == block_end)){
            return byte_cnt;
        }
    }
   
    return byte_cnt;
}


// /* 
//  * file_open_cp2
//  *   DESCRIPTION: open a regular file, actually do nothing
//  *   INPUTS: filename - file name
//  *   OUTPUTS: none
//  *   RETURN VALUE: inode index on success, -1 on failure
//  *   SIDE EFFECTS: none
//  */
// int32_t file_open_cp2(const uint8_t* filename){
//     dentry_t new_dentry;
//     // check the existence of the file
//     if(read_dentry_by_name(filename, &new_dentry)!= 0) return -1;

//     return new_dentry.inode;
// }
// /* 
//  * file_close_cp2
//  *   DESCRIPTION: close a regular file, actually do nothing
//  *   INPUTS: fd -- file descriptor 
//  *   OUTPUTS: none
//  *   RETURN VALUE: 0 
//  *   SIDE EFFECTS: none
//  */
// int32_t file_close_cp2(int32_t fd) {
//     printf("File closed (conceptually)\n");
//     return 0;
// }
// /* 
//  * file_read_cp2
//  *   DESCRIPTION: read content of regular file
//  *   INPUTS: fd - file discriptor !!!(later version)
//  *           idx_inode - inode index (cp2 version)
//  *           buf - store the content of reading
//  *           nbytes - number of bytes to read. 
//  *   OUTPUTS: filled buf, modified offset.
//  *   RETURN VALUE: bytes read on success, -1 on failure
//  *   SIDE EFFECTS: none 
//  */
// int32_t file_read_cp2(int32_t idx_inode, uint32_t* offset_pointer, uint8_t* buf, int32_t nbytes){
//     // sanity check
//     if( buf == NULL) return -1;

//     int32_t length;

//     length = read_data(idx_inode, *offset_pointer, buf, nbytes);
//     if (length > 0) {*offset_pointer += length;}

//     return length;

// }

// /* 
//  * file_write_cp2
//  *   DESCRIPTION: write content to regular file, but this is a read only file system
//  *   INPUTS: fd - file descriptor
//  *           buf - content to write to file
//  *           nbytes - length of content
//  *   OUTPUTS: none
//  *   RETURN VALUE: -1
//  *   SIDE EFFECTS: none
//  */
// int32_t file_write_cp2(int32_t fd, const void* buf, int32_t nbytes) {
//     return -1;
// }


// /* 
//  * direct_open_cp2
//  *   DESCRIPTION: open a directory
//  *   INPUTS: directname - name of directory
//  *   OUTPUTS: none
//  *   RETURN VALUE: 0 on success, -1 on anything bad happened
//  *   SIDE EFFECTS: none
//  */
// int32_t dir_open_cp2(const uint8_t* filename) {
//     dentry_t new_dentry;
//     // check the existence of the file
//     if(read_dentry_by_name(filename, &new_dentry)!= 0) return -1;

//     return 0;
// }

// /* 
//  * direct_close_cp2
//  *   DESCRIPTION: do nothing
//  *   INPUTS: fd - file descriptor
//  *   OUTPUTS: none
//  *   RETURN VALUE: 0
//  *   SIDE EFFECTS: none
//  */
// int32_t dir_close_cp2(int32_t fd) {
//     printf("Directory closed (conceptually) \n");
//     return 0;
// }

// /* 
//  * direct_read_cp2
//  *   DESCRIPTION: read the file name (it is user's responsible to ensure that buffer is larger than 33)
//  *   INPUTS: idx_dir - pointer to the index of target directory
//  *           buf - buffer that store the data to read
//  *           nbytes - number of bytes to read, not used.
//  *   OUTPUTS: buf storing the file name, incremented *idx_dir
//  *   RETURN VALUE: 0 for success, -1 for failure
//  *   SIDE EFFECTS: none
//  */
// int32_t dir_read_cp2(int32_t* idx_dir, void* buf, int32_t nbytes) {

//     dentry_t new_dentry;   // store temp dentry in loop

//     if (buf == NULL) {
//         return -1;
//     }

//     if (0 != read_dentry_by_index(*idx_dir, &new_dentry)) {
//         return -1;
//     }
//     (*idx_dir) += 1;

//     uint8_t* buffer = buf;
//     // Read file name
//     strncpy((int8_t*)buf, (int8_t*)new_dentry.f_name, 33);
//     buffer[32] = '\0';

//     return 0;
// }

// /* 
//  * direct_write_cp2
//  *   DESCRIPTION: do nothing, read only file system
//  *   INPUTS: fd - file descriptor
//  *           buf - buffer that store the data to write
//  *           nbytes - number of bytes to write
//  *   OUTPUTS: none
//  *   RETURN VALUE: -1
//  *   SIDE EFFECTS: none
//  */
// int32_t dir_write_cp2(int32_t fd, const void* buf, int32_t nbytes) {
//     return -1;
// }



/* 
 * file_open
 *   DESCRIPTION: open the file
 *   INPUTS: filename - file name
 *   OUTPUTS: none
 *   RETURN VALUE: 0 
 *   SIDE EFFECTS: none
 */
int32_t file_open(const uint8_t* filename) {
    return 0;
}

/* 
 * file_close
 *   DESCRIPTION: close the file
 *   INPUTS: fd -- file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 
 *   SIDE EFFECTS: none
 */
int32_t file_close(int32_t fd) {
    return 0;
}


/* 
 * file_read
 *   DESCRIPTION: read content of file
 *   INPUTS: fd - file discriptor
 *           buf - store the content of reading
 *           nbytes - number of bytes to read
 *   OUTPUTS: nbytes of file contents
 *   RETURN VALUE: bytes read on success, -1 on failure
 *   SIDE EFFECTS: none 
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
    // return value correct
    // sanity check
    if (buf == NULL) {return -1;}

    int32_t bytes_read;
    
    // get PCB
    pcb_t* pcb_ptr = get_pcb_ptr(pid);

    bytes_read = read_data(pcb_ptr->fds[fd].idx_inode, pcb_ptr->fds[fd].fseek, (uint8_t*)buf, nbytes);
    if (bytes_read == -1) {return -1;}

    //read success, update the info of pcb
    pcb_ptr->fds[fd].fseek += bytes_read;

    return bytes_read;


}

/* 
 * file_write
 *   DESCRIPTION: write content to file (but should do nothing in this mp)
 *   INPUTS: fd - file descriptor
 *           buf - content to write to file
 *           nbytes - length of content
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}


/* 
 * direct_open
 *   DESCRIPTION: open a directory
 *   INPUTS: dirname - name of directory
 *   OUTPUTS: none
 *   RETURN VALUE: 0 
 *   SIDE EFFECTS: none
 */
int32_t dir_open(const uint8_t* dirname) {
    return 0;
}

/* 
 * direct_close
 *   DESCRIPTION: do nothing
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t dir_close(int32_t fd) {
    return 0;
}

/* 
 * direct_read
 *   DESCRIPTION: read the file name (user should ensure that buffer is larger than 33)
 *   INPUTS: fd - file descriptor
 *           buf - buffer that store the data to read
 *           nbytes - number of bytes to read
 *   OUTPUTS: buf which stores the file name
 *   RETURN VALUE: bytes read on success, 0 if reach the end, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {

    // sanity check
    if (buf == NULL) {return -1;}

    dentry_t new_dentry;
    uint8_t* temp = buf;
    
    // Find the PCB
    pcb_t* pcb_ptr = get_pcb_ptr(pid);
    // read
    if ( 0 != read_dentry_by_index(pcb_ptr->fds[fd].fseek, &new_dentry)){
        // the end of directory is reached, repeatedly return 0
        return 0;
    }
    // read success, increment the fseek (index for dir_entry)
    pcb_ptr->fds[fd].fseek += 1;

    // copy back, set the end as '\0'
    strncpy((int8_t*)buf, (int8_t*)(new_dentry.f_name), 33);
    temp[32] = '\0';
    
    return strlen_xjz((uint8_t*)buf);

}

/* 
 * direct_write
 *   DESCRIPTION: do nothing
 *   INPUTS: fd - file descriptor
 *           buf - buffer that store the data to write
 *           nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}

/* 
 * get_size
 *   DESCRIPTION: get size of file
 *   INPUTS: inode - number of inode
 *   OUTPUTS: none
 *   RETURN VALUE: size of file if success, -1 if anything bad happened
 *   SIDE EFFECTS: none
 */
int32_t get_size(uint32_t inode) {
    return p_inode[inode].length;
}





