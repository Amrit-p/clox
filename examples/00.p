var first = 0;
var second = 1;
var next = 0;
var n = 50;
var i = 0;

while(i < n)
{
    print first;
    next = first + second;
    first = second;
    second = next;
    ++i;
}
