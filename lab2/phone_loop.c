#include <stdio.h>
#include <stdlib.h>

int main() {
	char phone[11];
	int num;
	int threw_error = 0;
	scanf("%s", phone);
	while (scanf("%d", &num) != EOF) {
		if (num == -1) {
			printf("%s\n", phone);
		} else if (num >= 0 && num <= 9) {
			printf("%c\n", phone[num]);
		} else if (num < -1 || num > 9){
			printf("ERROR\n");
			threw_error = 1;
		}	
	}
	return threw_error;
}