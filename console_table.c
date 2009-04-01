/** @file
 * @brief ����̨��ӡ��� (print tables in console application)
 * Project Name:none
 * <br>
 * Module Name:none
 * <br>
 * @author soulmachine@gmail.com
 * @date 2009/04/01
 * @version 0.1
 * @note none
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEAD 0
#define MIDDLE 1
#define TAIL 2

/**
 * @brief ����̨��ӡ��� (print tables in console application)
 * @param[in] location ����ߵ�λ�ã� 0 ��ͷ�ߣ�1 �����ߣ�2 ��β��
 * @param[in] col_count ��������
 * @param[in] col__widthes ����ÿ�еĿ��
 * @return �ɹ�����0��ʧ�ܷ��ش������
 * @note ��
 * @remarks ��
 */
int print_table_line(int location, int col_count, int *col__widthes)
{
    //ע�⣺��Щ�����ķ��ţ�����Ҫ��3���ַ�װ(����\0)
    char line_head[][3] = {"��", "��", "��"};
    char line_mid1[][3] = {"��", "��", "��"};
    char line_mid2[][3] = {"��", "��", "��"};
    char line_tail[][3] = {"��", "��", "��"};

    int i = 0;

    printf("%s",line_head[location]); //����

    for (i = 0; i < col_count; i++) {
        int j = 0;
        for (j = 0; j < col__widthes[i] / 2; j++) {
            printf("%s",line_mid1[location]);
        }

        if (i < col_count - 1)
            printf("%s", line_mid2[location]);
    }

    printf("%s\n", line_tail[location]);//��β

    return 0;
}


/** @brief �õ�һ�������Ŀ�� */
static int get_integer_width(void *e)
{
    int temp = *(int*)e;
    int width = 0;
    while (temp)
    {
        temp /= 10;
        width++;
    }
    return width;
}


/**
 * @brief ����ά�����ӡ�ɱ����ʽ
 * @param[in] a ��ά������
 * @param[in] row_count ����
 * @param[in] col_count ����
 * @return �ɹ�����0��ʧ�ܷ��ش������
 * @note ��
 * @remarks ��
 */
int print_array(int* a, int row_count, int col_count, int(*get_elem_width)(void *e))
{
    int i = 0, j = 0;
    int *col_width = (int*)malloc(col_count * sizeof(int));
    if(NULL == col_width)
    {
        return -1;
    }
    memset(col_width, 0, col_count * sizeof(int));

    // get max width of each column
    for(i = 0; i < col_count; i++){
        for(j = 0; j < row_count; j++) {
            int temp_width = get_elem_width(&a[j * row_count + i]);
            col_width[i] = col_width[i] > temp_width ? col_width[i] : temp_width;
        }
    }

    for(i = 0; i < col_count; i++){
        if(col_width[i] % 2 != 0) {
            col_width[i]++; // ������Ϊ����������1�����ż��
        }
    }

    //��ӡ��ͷ��
    print_table_line(HEAD, col_count, col_width);

    //��ӡ�������
    for (i = 0; i < row_count; i++)
    {
        if (i > 0) print_table_line(MIDDLE, col_count, col_width); //��ӡ������
        printf("��");	//����
        for (j = 0; j < col_count; j++)
        {
            int k = 0;
            int elem_width = get_elem_width(&a[i * col_count + j]);
            int space_count = col_width[j] - elem_width;

            // ���д�ӡԪ��
            for(k = 0; k < space_count / 2; k++) {
                printf(" ");
            }

            printf("%d",a[i * col_count + j]);

            for(k = 0; k < space_count / 2; k++) {
                printf(" ");
            }
            if(0 != space_count % 2 ) {
                printf(" ");
            }


            if (j < col_count - 1 )
                printf("��"); // ��������
        }
        printf("��\n"); //��β
    }

    print_table_line(TAIL, col_count, col_width); //��ӡ��β

    free(col_width);

    return 0;
}


int main()
{
    int A[][3] = {{1, 2000, 3}, {4, 5, 35}, {123456789, 8, 9}};
    print_array(&A[0][0], 3, 3, get_integer_width);
    return 0;
}
