# arith

    
Correctly implemented:
    calculation.h/.c
        - correct mathematical formula for RGB <-> CV & CV <-> DCT 
          transformation
        - modular design for functions and helper functions
        - no memory leaks
        
    bitpack.c:
        - 4 shifting operation functions that avoids C's property of shifting
          by word's width
        - width test, field extraction, field update functions for 64-bits 
          signed and unsigned int 
        - supporting uint64_t & int64_t
        - no memory leaks
        
    compress40.c 
        - calls the corresponding compress or decompress given command from 
          40image.c
        - Compression and Decompression both works correctly.
        - Modular Design with extensive map functions and apply functions
        - output the compressed file or decompressed image to stdout
        - handling CREs
        - no memory leaks
        
    ppmdiff.c
        - Correctly implemented to calculate the difference between ppms.


      
Documents the architecture of your solutions:
        
        -- 40image.c handles the command line operations and call the
           corresponding compress or decompress functionality in compress40.c.
        
        -- compress40.c executes compression or decompression given command
           from 40image.c. Specifically, compress40.c calls helper functions 
           from calculation.h/.c & bitpack.c to execute RGB <-> CV, 
           CV <-> DCT transformations, as well as packing/unpacking info 
           between DCT values and 64-bits unsigned int containing a 32-bits
           word. Note that 32-bits word is stored in big-endian order. 
           
        -- bitpack.c has its interface bitpack.h already provided to us. The
           functions within this interface was used in compress40.c to pack the
           numbers(integers with certain bits) to consume less space. 
           
        -- calculation.c is implemented by compress40.c to hide the details 
           about calculation and shortens the lengthy compress40.c. This
           interface contains mathematical formula for RGB <-> CV & CV <-> DCT.
           
