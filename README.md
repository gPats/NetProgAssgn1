# NetProgAssgn1
Assignment 1 for Network programming
-text parsing
-argument store
-invalid command identification

List of functions used:
1. get_tokens: 
	returns: 
		char** of list of tokens. 
		int number of tokens
	input: user string char*
2. check_validity
	returns: int 
		negative on error
	parameters: 
		char** tokens
	recursively checks path for executable
3. check_operators: &, <, >, >>, |, ||, |||