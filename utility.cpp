/*
A minimal set of i/o functions. We avoid the standard library because of potential memory costs of doing so.
*/

static void print(const char * str, int bytes)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
	write(1, str, bytes);
#pragma GCC diagnostic pop 
}

static void print_err(const char * str, int bytes)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
	write(2, str, bytes);
#pragma GCC diagnostic pop 
}

static void print_num(uint64_t number)
{
	static char digits[] = "0123456789";
    bool started = false;
    for(uint64_t i = 10000000000000000000ULL; i; i/= 10)
    {
        uint64_t digit = number / i;
        if(digit || started)
        {
            print(digits + digit, 1);
            started = true;
        }
        number -= digit * i;
    }
    if(!started)
        print(digits, 1);
}