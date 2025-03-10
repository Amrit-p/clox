var n = 4564;
var i = 2;
var is_prime = true;

if(n <= 1)
    is_prime = false;
else
{
    while(i * i <= n)
    {
        if(n % i == 0)
        {
            is_prime=false;
        }
        ++i;
    }
}

if(is_prime)
{
    print n;
    print "is prime";
}
else 
{
    print n;
    print "is not prime number";
}