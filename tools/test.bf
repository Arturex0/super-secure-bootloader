+++++++++++++++++++++
>
+++++++++++++++++++++++++++++++++++++++++


// Input decryption keys 



// Input ciphertext



// this part can be handeled by 


>	Increment the cell pointer (move it to the right)	++ptr;
<	Decrement the cell pointer (move it to the left)	--ptr;
^   Increment the cell pinter by the value of current cell (move it to the right) ptr += *ptr;
v   Decrement the cell pinter by the value of current cell (move it to the left)  ptr -= *ptr;
+	Increment the value of the current cell	++*ptr;
-	Decrement the value of the current cell	--*ptr;
.	Write the value of the current cell to screen	putchar(*ptr);
,	Read one byte of input and store the value in the current cell	*ptr = getchar();
[	If the cell value is zero, jump to the command following the matching ] command	while (*ptr) {
]	If the cell value is not zero, jump back to the command after the matching [	}
0   jump to cell with pointer 0 ptr=0;





loop throug the main loop

  

             TO FROM
[TMP] [1] [8] [1] [6]  [8] [val1] [val0] [val2] [val4] [val3] [0] [val5] [val6] [val7] [val8] [val9] [val10]


1. move val 1 to temp
2. move val from [from] to [to]
3. update values +1 to [TO] +2 to [from]
4. move val from from to [to]


decrypted_byte = (encrypted_byte - key1 - (index * 2))

[TMP0] [TMP1] [addr] [key1] [key2] [index] [index2] [0] [2] [4] [6] [8] [10] [0] [1] [3] [5] [7] [9]


1. multiple index by 2 store it in index2 
2. move 
2. sub key 1 from encrypted bit
3. sub index2 from encrypted bit 
4. add 1 to inedx 
5. clear index2 
6. move to next bype possition
 


[0] [1] [8] [1] [6] [8] [val1] [val0] [val2] [val4] [val3] [0] [val5] [val6] [val7] [val8] [val9] [val10] 


1. move val to temp.
2. swap 


+2 


#

[ >




[key1] [key2] [ciphertext] [part1] [part2]






[1] [2] [3] [4] [5] [6] [7] [8] [9] [0] [0] []



[0][1][2][3][4][5] [6] [1] [2] [3] [4] [5] [6] [7] [8] [9] [0] [0]


