#include "lock_free_stack.h"


static void smoke_test() {
    LockFreeStack<int> st;
    st.push(0);
    st.push(1);
}

int main() {
    smoke_test();
    return 0;
}