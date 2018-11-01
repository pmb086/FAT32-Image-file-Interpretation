#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments 

#define NUM_BLOCKS 512

#define BLOCK_SIZE 1024

unsigned char file_data[NUM_BLOCKS][BLOCK_SIZE];
FILE *fp, *ofp;
short BPB_BytesPerSec;
unsigned char BPB_SecPerClus;
short BPB_RsvdSecCnt;
unsigned char BPB_NumFATS;
int BPB_FATSz32;
unsigned char BS_VolLab[11];
int root_offset;
int new_offset;
int mem_offset;

struct __attribute__((__packed__)) DirectoryEntry {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

int16_t NextLb (uint32_t sector)
{
  uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector*4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}

int LBAToOffset(int32_t sector)
{
  return (( sector - 2 ) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec);
}

int main()
{ 
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );//Dynamically allocating pointer memory
  int flag = 0, count = 0, leaf = 0, i,j  = 0;

  while( 1 )//infinite loop
  {
    start:
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    
    fgets (cmd_str, MAX_COMMAND_SIZE, stdin);
    if (strcmp (cmd_str, "\n") ==0)//To check if there is an input
           goto start;//transfers the line of control to start
        
/* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
 
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }
  if( strcmp(*token, "open") == 0)
  {
	  fp= fopen(token[1],"r");
	  if ( fp ==  NULL )
	  {
              printf("Error: File system image not found\n");
              continue;
          }
	  else if ( flag == 1)
          {
	       printf("Error: File system image already open\n");
               continue;
          }
          flag = 1;
          fseek(fp, 11, SEEK_SET);
          fread(&BPB_BytesPerSec, 2, 1, fp);
  
          fseek(fp, 13, SEEK_SET);
          fread(&BPB_SecPerClus, 1, 1, fp);
 
          fseek(fp, 14, SEEK_SET);
          fread(&BPB_RsvdSecCnt, 2, 1, fp);
 
          fseek(fp, 16, SEEK_SET);
          fread(&BPB_NumFATS, 1, 1, fp);
  
          fseek(fp, 36, SEEK_SET);
          fread(&BPB_FATSz32, 4, 1, fp);

          root_offset=( BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
 
  }

  else if(flag == 1 && strcmp(*token, "close") == 0)
  {
	  fclose(fp);
	  flag = 0;
  }

  else if(flag == 0 & strcmp(*token, "close") == 0)
  {
	  printf("Error: File system not open\n");
  }

  else if(flag == 1 && strcmp( *token, "info")==0)
  {
  printf("BPB_BytesPerSec: %d\n", BPB_BytesPerSec);
  printf("BPB_BytesPerSec: %x\n", BPB_BytesPerSec);
  
  printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
  printf("BPB_SecPerClus: %x\n", BPB_SecPerClus);
 
  printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);
  printf("BPB_RsvdSecCnt: %x\n", BPB_RsvdSecCnt);

  printf("BPB_NumFATS: %d\n", BPB_NumFATS);
  printf("BPB_NumFATS: %x\n", BPB_NumFATS);
 
  printf("BPB_FATSz32: %d\n", BPB_FATSz32);
  printf("BPB_FATSz32: %x\n", BPB_FATSz32);
  }

  else if( flag == 0 && strcmp(*token, "info") == 0)
  {
	  printf("Error: File system image must be opened first\n");
  }

  else if ( flag == 1 && strcmp(*token, "stat" ) == 0)
  {
     if( leaf == 0)
     fseek( fp, root_offset, SEEK_SET );
 
     for( i=0; i<16; i++)
     {
        memset( &dir[i], 0, 32);
        fread( &dir[i], 32, 1, fp );
     }

     for( i=0 ; i<16 ; i++)
     {
      if( dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20 )
      { 
           char name1[12];
           char name2[12];
           memset( name1, 0, 12);
           strncpy( name1, dir[i].DIR_Name, 11);
           for(i = 0; i<12 ; i++ )
           {
            if( name1[i] != ' ')
            {
            name2[j] = name1[i] ;
            j++;
            }
           }
           if( strcmp(token[1], name2) == 0)
           {
            printf("FileSize: %d  StartCluster Number: %d\n", dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
            count = 0;
           }
       }
       
       count++;
       if ( count == 15 ){
         printf("Error: File not found\n");
         count = 0;
       }
     }
  } 

  else if( flag == 0 && strcmp(*token, "stat") == 0)
  {
       printf("Error: File system image must be opened first\n");
  }
  
  else if ( flag == 1 && strcmp(*token, "ls" ) == 0)
  {
  if(leaf == 0)
  fseek( fp, root_offset, SEEK_SET );
  else
  {
    printf(".\n..\n");
    fseek( fp, new_offset, SEEK_SET );
  }
  for( i=0; i<16; i++)
  {
        fread( &dir[i], 32, 1, fp );
  }
  
  for( i=0; i<16; i++)
  {
   if( dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20 )
   {
    char name[12];
    memset( name, 0, 12 );
    strncpy( name, dir[i].DIR_Name, 11);
    printf("%s\n", name);
   }
  }
  }

  else if( flag == 0 && strcmp(*token, "ls") == 0)
  {
         printf("Error: File system image must be opened first\n");
  }
 
  else if( flag == 1 && strcmp(*token, "cd") == 0)
  {
     if(leaf == 0) 
     fseek( fp, root_offset, SEEK_SET );
     
     for( i=0; i<16; i++)
     {
        memset( &dir[i], 0, 32);
        fread( &dir[i], 32, 1, fp );
     }
  
       for( i =0 ; i<16 ; i++)
       {
         if( dir[i].DIR_Attr == 0x10 )
         {
           char name1[12];
           char name2[12];
           memset( name1, 0, 12);
           strncpy( name1, dir[i].DIR_Name, 11);
           for(i = 0; i<12 ; i++ )
           {
            if( name1[i] != ' ')
            {
            name2[j] = name1[i] ;
            j++;
            }
           }
           if( strcmp(token[1], name2) == 0)
           {           
            fseek(fp, dir[i].DIR_FirstClusterLow, SEEK_SET);
            new_offset = ftell(fp);
            leaf++;
           }
           else if( strcmp( token[1], "..") == 0)
           {
             mem_offset= dir[i].DIR_FirstClusterLow - root_offset;
             fseek(fp, mem_offset, SEEK_SET);
             leaf--;
           }
         }
       }
  } 
   
  else if ( flag == 0 && strcmp(*token, "cd") == 0)
  {
       printf("Error: File system image must be opened first\n");   
  }
 
  else if( flag == 1 && strcmp(*token, "get") == 0)
  {
    struct stat buf;
    int status =  stat( token[1], &buf );   
    ofp = fopen(token[1], "w");

    if( ofp == NULL )
    {
      printf("Could not open output file: %s\n", token[1] );
      perror("Opening output file returned");
      return -1;
    }

    int block_index = 0;
    int copy_size   = buf . st_size;
    int offset      = 0;

    while( copy_size > 0 )
    { 

      int num_bytes;

      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else 
      {
        num_bytes = BLOCK_SIZE;
      }

      fwrite( file_data[block_index], num_bytes, 1, ofp ); 

      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index ++;

      fseek( ofp, offset, SEEK_SET );
    }
    fclose( ofp );
  
  }
 
  else if( flag == 0 && strcmp(*token, "get") == 0)
  {
         printf("Error: File system image must be opened first\n");
  }
 
  else if( flag == 1 && strcmp(*token, "read") == 0)
  {
     fseek( fp, root_offset, SEEK_SET );
     int rd_count = atoi(token[2]);
 
     for( i=0; i<16; i++)
     {
        fread( &dir[i], 32, 1, fp );
     }
  
      for( i =0 ; i<16 ; i++)
      {
       if( dir[i].DIR_Attr == 0x10 )
       {   
         char name1[12], name2[12];
         memset( name1, 0, 12);
         strncpy( name1, dir[i].DIR_Name, 11);
         for(i = 0; i<12 ; i++ )
         {
            if( name1[i] != ' ')
            {
            name2[j] = name1[i] ;
            j++;
            }
         }  
         int file_offset = LBAToOffset(17); 
         uint8_t value;
       if( strcmp(token[1], name2) == 0)
       {      
       if( dir[i].DIR_FileSize <= 512 ){
       fseek( fp, file_offset, SEEK_SET );
       fread( &value, 1, 1, fp);
        for( i=1; i<rd_count; i++)
         {
          fread( &value, 1, 1, fp );
          printf("%d ", value);
         }
       }
       else
       {
       int user_offset = 513;
       int block = dir[i].DIR_FirstClusterLow;
       while( user_offset > BPB_BytesPerSec )
          {
           block = NextLb( block );
           user_offset -= BPB_BytesPerSec;
          }
       
       file_offset = LBAToOffset( block );
       fseek( fp, file_offset + user_offset, SEEK_SET);
       for( i=1; i<rd_count; i++)
         {
          fread( &value, 1, 1, fp );
          printf("%d ", value);
         }
       }  
       }
       }
      }
 }
 
  else if ( flag == 0 && strcmp(*token, "read") == 0)
  {
       printf("Error: File system image must be opened first\n");   
  }
  else if( flag == 1 && strcmp(*token, "volume") == 0)
  {
     fseek(fp, 43, SEEK_SET);
     fread(&BS_VolLab, 1, 1, fp);
     printf("Volume label: %x\n",BS_VolLab);
     if( strcmp( BS_VolLab, "NO NAME	") == 0)
         printf("Error: volume name not found.\n");
  }
  
  else if(flag == 0 && strcmp(*token, "volume") == 0)
  {
        printf("Error: File system image must be opened first\n");
  }
}
  return 0;
}
