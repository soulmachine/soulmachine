#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "utility.h" /* for fsize() */

#define CHILDREN_BYTES_COUNT 256 /* һ�������У�һ���ֽڵ���һ���ֽڵ������� */
#define BUFFERSIZE 1024 /* ��ȡҪ���зִʵ��ı��ļ�ʱ�������Ĵ�С����һ�ζ�ȡ�����ֽ�*/
#define MAXLENGTHOFWORD 36 /* �����ĳ��� */
#define GB2312NUM   72*94 /* GB2312�ĺ��ֵĸ��� */
#define NODE_DEFAULT_HIT_COUNT 10 /* Ĭ�����ֻ��10�����ڵĳ��ִ��������������ţ������ö��ڴ� */



/* Trie���Ľڵ� */
typedef struct tagTreeNode
{
        unsigned int occurence;/* ���ֵĴ��������Դ���ż������ӽڵ���Ч����search����һ�Σ�����1 */
        struct tagTreeNode* children[256]; /* TRUE ��ʾ��Ӧ�ĺ����Ƿ��к����ֽڣ�û�о���Ҷ�� */
}TreeNode;

/* ��������� */
typedef struct tagFirstWord
{
        unsigned char word[2];/* �������λ������postTraverse����д������ļ� */
        TreeNode* children[CHILDREN_BYTES_COUNT]; /* TRUE ��ʾ��Ӧ�ĺ����Ƿ��к����ֽڣ�û�о���Ҷ�� */
}FirstWord;


/** @see cws_hit_info_t */
typedef struct cws_hit_info_t cws_hit_info_t;

/** ���һ���ؼ�����һ���ĵ��е�������Ϣ */
struct cws_hit_info_t {
        int hit_count;      /**< �������ı��г��ֵĴ��� */
        struct {
                int begin,end;
        }*positions;       /**< ����Ǹ��ؼ��ʳ��ֵ�λ�� */
};

/** @see cws_forward_index_t */
typedef struct cws_forward_index_t cws_forward_index_t;

/** ���������Ľṹ���� */
struct cws_forward_index_t {
        unsigned int doc_id;       /**< һ���ĵ��ı�� */
        struct {
                unsigned int word_id;
                cws_hit_info_t *hits;
        };
};

/** @see cws_dict_node_t */
typedef struct cws_dict_node_t cws_dict_node_t;

/** �ֵ�����һ���ڵ� */
struct cws_dict_node_t {
        unsigned int word_id;    /**< ���ʵı�ţ�-1 ��ʾ����ЧID�����ӵ�һ�㵽������ֽڻ�������һ������ */
        cws_dict_node_t *children[CHILDREN_BYTES_COUNT];         /**< ���ӽڵ� */
};



FirstWord firstWords[GB2312NUM];/* �����д�������ֵļ��� */
/* ���ݺ��ֵ���λ��ȡ������ֵ������WinHex�۲죬�ļ��е�һ���ֽ�Ϊ���ţ��ڶ����ֽ�Ϊλ�� */
int wordToIndex(unsigned char qu, unsigned char wei)
{
        if(!((qu >= 0xB0) && (qu <= 0xF7)))
        {
                return -1;
        }
        if(!((wei >= 0xA1) && (wei <= 0xFE)))
        {
                return -1;
        }
        return (qu - 176) * 94 + (wei - 161);
}

/**
* @brief ���ֵ��ļ���ȡ�������ֵ���
* @author soulmachine@gmail.com
* @param[in] fpDict �ֵ��ļ�
* @return 
* @note detailed description
* @remarks 
* @par Modification History
* -------------------------------------- <br>
* Date          Name        Desciption <br>
* -------------------------------------- <br>
* YYYY/MM/DD
*/
UINT32 CreateDictTree(VOID)
{
        return 0;
}
/* ���ֵ��ļ���ȡ������Trie�� */
/* firstWords��ʾ���ֵ����飬num��ʾ����Ĵ�С������GB2312��numΪGB2312NUM */
/**
* @brief ���ֵ��ļ���ȡ������Trie��
* @author soulmachine@gmail.com
* @param[in] fpDict �ֵ��ļ�
* @param[in] pFirstWords FirstWord�����׵�ַ
* @return 
* @note ��
* @remarks ��
*/
bool createTrieTree(FILE* fpDict, FirstWord* pFirstWords)
{
        size_t nDictSize = 0; /* �ֵ��ļ��Ĵ�С */
        size_t readCount = 0; /* fread ���ص�ʵ�ʶ�ȡ���ֽ��� */
        unsigned char* pbDictBuffer;/* �������dict�ļ��Ļ����� */
        size_t i = 0, j = 0; /* ѭ������ */
        FirstWord* currentWord= NULL; /* ���ڴ�������ֽڵ� */
        TreeNode** currentNode= NULL; /* ���ڴ�������ڵ� */

        debug_printf("in function createTrieTree\n");

        assert(NULL != fpDict);
        assert(NULL != pFirstWords);

        nDictSize = fsize(fpDict);
        pbDictBuffer = (unsigned char*)malloc(sizeof(unsigned char) * nDictSize);
        if(NULL == pbDictBuffer)
        {
                debug_printf("malloc failed at lint %d\n",__LINE__);
                goto malloc_failed;
        }
        memset(pbDictBuffer,0,nDictSize);

        readCount = fread(pbDictBuffer, sizeof(unsigned char), nDictSize, fpDict); //��ȡ�ֵ䵽dict����
        assert(readCount == nDictSize);

        debug_printf("line %d:\tdictSize = %u\n", nDictSize);

        while (i < nDictSize)
        {
                int index = wordToIndex(pbDictBuffer[i],pbDictBuffer[i+1]);
                if(index < 0)
                {
                        // ������һ�еĴ�
                        while(pbDictBuffer[++i] != 0x0d);
                        i+=2;//���� 0x0a
                        continue;
                }
                else
                {
                        currentWord = &(pFirstWords[index]);
                }

                debug_printf("line %d:\tstart firstWord[%x,%x]\n",__LINE__,pbDictBuffer[i],pbDictBuffer[i+1]);

                (*currentWord).word[0] = pbDictBuffer[i];
                (*currentWord).word[1] = pbDictBuffer[i + 1];
                /* children ��main�г�ʼ���� */
                i += 2; /* ��ʼ�������ĵ������ֽ� */
                if(pbDictBuffer[i] != 0x0d)
                {
                        currentNode = &(currentWord->children[pbDictBuffer[i]]);
                        if(*currentNode == NULL)
                        {
                                *currentNode = (TreeNode*)malloc(sizeof(TreeNode)); /*ָ���Ϊ��NULL����ʾ������һ������ */
                                assert(*currentNode != NULL);
                                /* ��ʼ�� */
                                (*currentNode)->occurence = 0;
                                for (j = 0; j < 256; ++j)
                                {
                                        (*currentNode)->children[j] = NULL; /* ��ʾ��û�к����ֽ� */
                                }

                        }

                        debug_printf("line %d:\tfirstWord[%x,%x]->children[%x] = %x\n",__LINE__,pbDictBuffer[i - 2],pbDictBuffer[i - 1],pbDictBuffer[i],*currentNode);

                        ++i;
                }
                else
                {/* ��ʱ����ض��Ǹ�0x0a�������������µ�ѭ�� */
                        i += 2;
                        continue;
                }
                /* ��ʼ������ĸ��ֽ��Լ������ֽ� */
                while (0x0d != pbDictBuffer[i])
                {
                        /* ���һ���ӽڵ� */
                        currentNode = &((*currentNode)->children[pbDictBuffer[i]]);/* �ڸ��ڵ��ҵ���Ӧ��λ�� */
                        if(NULL == *currentNode)
                        {
                                *currentNode = (TreeNode*)malloc(sizeof(TreeNode)); /*ָ���Ϊ��NULL����ʾ������һ������ */
                                assert(*currentNode != NULL);
                                /* ��ʼ�� */
                                (*currentNode)->occurence = 0;
                                for (j = 0; j < 256; ++j)
                                {
                                        (*currentNode)->children[j] = NULL; /* ��ʾ��û�к����ֽ� */
                                }
                        }

                        debug_printf("line %d:\tchildren[%x]  = %x\n",__LINE__,pbDictBuffer[i],*currentNode);

                        ++i;
                }
                /* ��ʱ����ض��Ǹ�0x0a�������µ�ѭ�� */
                i += 2;
        };

        free(pbDictBuffer);
        pbDictBuffer = NULL;

        debug_printf("leave function createTrieTree\n");
malloc_failed:
        return FALSE;

}
/* ǰ�������ɾ�������ڵ㣬��ӡ��� */
void preTraverse(FILE* const resultFile,const TreeNode** const node,unsigned char indexOfNode,const unsigned char* const firstTwoBytes)
{
        static unsigned char stack[MAXLENGTHOFWORD];/*����ջ���� */
        static int top = 0; /* ջ��*/
        size_t i = 0;

        assert(NULL != resultFile);

        if(NULL != *node)
        {
                *(stack + top++) = indexOfNode;
                if((*node)->occurence != 0)
                {
                        fwrite(firstTwoBytes,sizeof(unsigned char),2,resultFile);
                        fwrite(stack,sizeof(unsigned char),top,resultFile);
                        fprintf(resultFile,"\t\t\t%d\r\n",(*node)->occurence);
                }

                /* �ж��ǲ���Ҷ�ӽڵ�*/
                for (i = 0; i < 256; ++i)
                {
                        if (NULL != (*node)->children[i])
                        {
                                break;
                        }
                }
                if (256 == i)
                { /*��Ҷ�ӽڵ� */
                        --top;

                        //free(*node);
                        free((TreeNode *)*node);
                        *node = NULL;
                }


                else
                {
                        for (i = 0; i < 256; ++i)
                        {
                                if ((*node)->children[i] != NULL)
                                        preTraverse(resultFile,(const TreeNode **const)&((*node)->children[i]),i,firstTwoBytes);
                        }
                }
        }
}
/* ����ÿ������������� */
void traverseTrees(FILE* const resultFile,const FirstWord* const firstWords)
{
        size_t i = 0, j = 0;
        for (i = 0; i < GB2312NUM; ++i)
        {
                for (j = 0; j < 256; ++j)
                        preTraverse(resultFile,(const TreeNode **const)&(firstWords[i].children[j]),j,firstWords[i].word);
        }
}

/* ����Ƿ��Ǻ��ֵĵ�һ���ֽ� */
BOOL firstIsWord(unsigned char ch)
{
        if (ch>=0xb0&&ch<=0xf7)
        {
                return TRUE;
        }
        else
        {
                return FALSE;
        }
}

/* ����Ƿ��Ǻ��ֵĵڶ����ֽ� */
BOOL secondIsWord(unsigned char const ch)
{
        if (ch>=0xa1&&ch<=0xfe)
        {
                return TRUE;
        }
        else
        {
                return FALSE;
        }
}
/* �ж������ֽ��ǲ��ǹ���һ������ */
BOOL isAChinese(unsigned char const first, unsigned char const second)
{
        if (firstIsWord(first) && secondIsWord(second))
        {
                return TRUE;
        }
        else
        {
                return FALSE;
        }
}
/* �ж��Ƿ��ǵ���*/
BOOL isSingle(unsigned char firstByte,unsigned char secondByte,unsigned char followedByte){
        return(NULL == firstWords[wordToIndex(firstByte,secondByte)].children[followedByte]);
}

/* ����Ƿ���Ӣ����ĸ */
BOOL isEnglish(unsigned char const ch)
{
        if ((ch>='a' && ch <='z') || (ch>='A' && ch <='Z'))
        {
                return TRUE;
        }
        else
        {
                return FALSE;
        }
}

/* ������Ϊ������ţ��Ͼ��ã�֮��Ͷ����� */
BOOL isSymbol(unsigned char const ch)
{
        if (!firstIsWord(ch) && !secondIsWord(ch) && !isEnglish(ch))
        {
                return TRUE;
        }
        else
        {
                return FALSE;
        }
}
/* �����Ĵ���������һ������ ������ֵ�ı�ʾ��һ��������ֽ���  */
/* startָ����ֽڱ�����һ������Ŀ�ʼ��endָ����ֽ��Ǵ���Ľ�β�ĺ�һ���ֽ�*/
/* ��segmentation ��֤��� */
/* start  �� segmentation ���ݹ��������ǵ��ֻ��������֣���start �� end ָ����ֽ�������һ����������
*  ����ֵ��ʾƥ�䵽�Ĵ������ֽ���������Ϊ4
*/
unsigned int searchFirstWord(const unsigned char* start, const unsigned char* const end)
{
        TreeNode *node;
        const unsigned char* index = start;
        size_t i = 0;/* ��������330 ���е�for ѭ��*/
        assert(isAChinese(*start,*(start + 1)));

        debug_printf("in function search\n");
        debug_printf("line %d:\tstart firstWord[%x,%x]\n", __LINE__,*start,*(start + 1));

        node = firstWords[wordToIndex(*start,*(start + 1))].children[*(start + 2)];
        assert(NULL != node);

        debug_printf("lint %d:\tfirstWord[%x,%x].children[%x] = %x\n",__LINE__,*start,*(start + 1),*(start + 2),node);


        for (index = start + 3; index < end; index += 2)
        {/*node ָ���ֵĵ�һ���ֽڣ�iָ���ֵĵڶ����ֽ� */

                assert(isAChinese(*(index-1),*index));

                debug_printf("line %d:\t%x %x\n",__LINE__,*index,*(index+1));

                if (node != NULL)
                {
                        debug_printf("line %d:\t%x->children[%x] = %x\n",__LINE__,node,*index,node->children[*index]);

                        node = node->children[*index]; /* ����һ�� */


                        if (node != NULL)
                        {
                                debug_printf("line %d:\t%x->children[%x] = %x\n",__LINE__,node,*(index + 1),node->children[*(index + 1)]);

                                /* �ж�node �Ƿ���Ҷ�ӽڵ� */
                                for(i = 0; i < 256; ++i){
                                        if(NULL != node->children[i]){
                                                break;
                                        }
                                }
                                if(256 == i){
                                        ++node->occurence;
                                }
                                node = node->children[*(index + 1)];
                        }
                        else
                        {/*ż���ֽ�����Trie���У������ֽ�ȴ����(��0��ʼ)��Ҳ����˵һ������ֻ��"һ��"��Trie ���У������ܷ��� */
                                /* ������ִ�е��˴� */
                                fprintf(stderr,"fatal error in trie!\n");
                                exit(-1);
                        }
                }
        }

        debug_printf("leave function search\n");

        assert(1 == index - end); /* ��ʱindex �϶���end����*/
        return index - start - 1;
}


/* ��һ���Ϸ��Ĵ���������ƥ�䣬�Ӵ���ĵڶ������ֵ������ڶ������֣�
* ��Ϊ���Ƕ��п�����һ������Ŀ�ͷ
* startOfWord��ʾһ������ĵ�һ���ֽڣ�endOfWord ָ��������һ���ֽڵ���һ�ֽ�
*/
void wordOfWord(const unsigned char* startOfWord, const unsigned char* const endOfWord)
{
        for(startOfWord += 2; startOfWord < endOfWord - 4;startOfWord += 2)
        {
                (void)searchFirstWord(startOfWord,endOfWord);
        }
}
void segmentation(FILE* const resultFile,FILE* const englishFile,FILE* textFile)/* ���ķִ������� */
{
        /* Ϊ����ʶ��"�л����񹲺͹�"�е�"����"��"���񹲺͹�"��"���͹�"����Ҫ��������һ��embededStartָ�� */
        /* unsigned char* start,end,embededStart; */
        unsigned char* buffer;/*fread �Ļ����� */
        unsigned int length = 0; /* ÿ��ƥ�䵽�Ĵ���ĳ��� */
        size_t textFileOffset = 0;
        size_t bufferStart = 0; /* ���������Ӣ��*/
        size_t bufferEnd = 0;
        size_t chineseBufferStart = 0; /* buffer + chineseBufferStart �� */
        size_t chineseBufferEnd = 0; /* buffer + chineseBufferEnd ֮����ֽ����ᴫ�ݸ�searchFirstWord */
        BOOL hasSecondConfused = FALSE;

        assert(NULL != resultFile);
        assert(NULL != englishFile);

        buffer = (unsigned char*)malloc(sizeof(unsigned char) * BUFFERSIZE);
        if (NULL == buffer)
        {
                fprintf(stderr,"malloc failed at lint %d\n",__LINE__);
                exit(-1);
        }
        memset(buffer,0,BUFFERSIZE);
        while (0 == feof(textFile))
        {
                bufferStart = 0;
                bufferEnd = 0;
                if ((textFileOffset
                        = fread(buffer, sizeof(unsigned char),
                        BUFFERSIZE/*Ҫȷ����������װ��һ�������Ĵ�����뿼�Ǽ����������������β����0x0d0a */,
                        textFile)) > 0)
                {/* ��Ҫ�߼��������� */
                        /*����һ�λ���ȡ�����÷Ǻ��֣���Ӣ�ĺ�������ŶϾ� */

                        debug_printf("line %d:\t textFileOffset = %d\r\n",__LINE__,textFileOffset);

                        while(bufferEnd < textFileOffset - 1) /* buffer ���һ���ֽ���Ϊ�ڱ� */
                        {
                                length = 0;

                                debug_printf("line %d:\t start while with:%x,%x\r\n",__LINE__,*(buffer + bufferEnd),*(buffer + bufferEnd + 1));

                                while(isSymbol(*(buffer + bufferEnd)))
                                {/* ���Է���*/
                                        ++bufferEnd;
                                }
                                bufferStart = bufferEnd;

                                while(isEnglish(*(buffer + bufferEnd)))
                                {
                                        ++bufferEnd;
                                }
                                fwrite(buffer + bufferStart,sizeof(unsigned char),bufferEnd - bufferStart,englishFile);
                                bufferStart = bufferEnd;

                                /* ����ǰ�������Լ�����*/
                                while(firstIsWord(*(buffer + bufferEnd)))
                                {
                                        ++bufferEnd;
                                        if(secondIsWord(*(buffer + bufferEnd)))
                                        {
                                                ++bufferEnd;
                                                if(isSingle(*(buffer + bufferEnd - 2),*(buffer + bufferEnd - 1),*(buffer + bufferEnd))){
                                                        continue;
                                                }
                                                else{
                                                        chineseBufferStart = bufferEnd - 2;
                                                        break;
                                                }
                                        }
                                        else/* �ڶ��ֽ�������*/
                                        {
                                                ++bufferEnd;/* ����*/
                                        }
                                }

                                while(firstIsWord(*(buffer + bufferEnd))){
                                        ++bufferEnd;
                                        if(secondIsWord(*(buffer + bufferEnd)))
                                        {
                                                ++bufferEnd;
                                        }
                                        else/* �ڶ��ֽ�������*/
                                        {
                                                hasSecondConfused = TRUE;
                                                ++bufferEnd;/* ����*/
                                        }
                                }
                                chineseBufferStart = bufferStart;
                                chineseBufferEnd = hasSecondConfused?(bufferEnd - 2):bufferEnd;
                                if(bufferStart < bufferEnd - 1){
                                        length = searchFirstWord(buffer + chineseBufferStart,buffer + chineseBufferEnd);}

                                debug_printf("line %d:\t length = %d,bufferEnd = %d\r\n",__LINE__,length,bufferEnd);

                                bufferStart = bufferEnd;
                        }
                }
                else
                {
                        break;
                }
        }
        free(buffer);
        buffer = NULL;
}
int main(int argc, char* argv[])
{
        FILE* dictFile = fopen(argv[1], "rb");
        FILE* honglou = fopen(argv[2], "rb");
        FILE* resultFile = fopen("result.txt","wb");
        FILE* englishFile = fopen("english.txt","wb");
        int i = 0, j = 0;

        if(argc != 3)
        {
                printf("usagge:\t%s\t<dict> <text file>\n");
                return 0;
        }

        for (i = 0; i < GB2312NUM; ++i)
        {
                firstWords[i].word[0] = 0;
                firstWords[i].word[1] = 0;
                for (j = 0; j < 256; ++j)
                {
                        firstWords[i].children[j] = NULL;
                }
        }
        if (NULL == dictFile)
        {
                fprintf(stderr, "Can't open %s !\n", argv[1]);
                return -1;
        }
        if (NULL == resultFile)
        {
                fprintf(stderr, "Can't create %s !\n", argv[2]);
                return -1;
        }
        if (NULL == englishFile)
        {
                fprintf(stderr, "Can't create english.txt!\n");
                return -1;
        }
        createTrieTree(dictFile, firstWords);
        segmentation(resultFile,englishFile,honglou);
        traverseTrees(resultFile,firstWords);
        fclose(dictFile);
        fclose(honglou);
        return (0);
}

