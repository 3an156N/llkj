#include <stdio.h>
#include <string.h>  // 包含 strcspn 函数所需头文件

int main() {
    char name[100];
    
    printf("Hello, World!\n");
    printf("请问怎么称呼您？");
    
    // 使用fgets可以安全地读取包含空格的输入
    fgets(name, sizeof(name), stdin);
    
    // 移除fgets读取的换行符
    name[strcspn(name, "\n")] = '\0';
    
    printf("很高兴认识你, %s!\n", name);
    
    return 0;
}
