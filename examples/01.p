
function fibonacci(n)
{
    if (n <= 0) return 0; // Base case: Fibonacci(0) = 0
    if (n == 1) return 1; // Base case: Fibonacci(1) = 1
    return fibonacci(n - 1) + fibonacci(n - 2);
}

var x = fibonacci(40);
print x;