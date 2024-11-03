#include "thread.h"

unsigned int balance = 100;

int Alipay_withdraw(int amt) {
    if (balance >= amt) {
        balance -= amt;
        return SUCCESS;
    } else {
        return FAIL;
    }
}
int main () {


}