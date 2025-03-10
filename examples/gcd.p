var num1 = 34;
var num2 = 60;
var a = num1;
var b = num2;

while(b != 0)
{
    if(a > b)
    {
        a = a - b;
    }else{
        b = b - a;
    }
}

print "GCD of a&b is";
print a;