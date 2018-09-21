auto  a = chrono::high_resolution_clock::now();
function();
auto b = chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(b - a);
cout << duration.count() << "\n";