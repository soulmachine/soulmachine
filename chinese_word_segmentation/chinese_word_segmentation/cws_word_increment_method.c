#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "config.h"
#include "utility.h" /* for fsize() */

#define CHILDREN_BYTES_COUNT 256 /* 一个单词中，一个字节的下一个字节的最大个数 */
#define BUFFERSIZE 1024 /* 读取要进行分词的文本文件时缓冲区的大小，即一次读取多少字节*/
#define MAXLENGTHOFWORD 36 /* 最长词语的长度 */
#define GB2312NUM   72*94 /* GB2312的汉字的个数 */
#define NODE_DEFAULT_HIT_COUNT 10 /* 默认如果只有10个以内的出现次数，就用数组存放，否则用堆内存 */



/* Trie树的节点 */
typedef struct tagTreeNode
{
        unsigned int occurence;/* 出现的次数，仅对处于偶数层的子节点有效，当search命中一次，就增1 */
        struct tagTreeNode* children[256]; /* TRUE 表示对应的孩子是否有后续字节，没有就是叶子 */
}TreeNode;

/* 词语的首字 */
typedef struct tagFirstWord
{
        unsigned char word[2];/* 存放区和位，仅被postTraverse用来写结果到文件 */
        TreeNode* children[CHILDREN_BYTES_COUNT]; /* TRUE 表示对应的孩子是否有后续字节，没有就是叶子 */
}FirstWord;


/** @see cws_hit_info_t */
typedef struct cws_hit_info_t cws_hit_info_t;

/** 存放一个关键词在一个文档中的命中信息 */
struct cws_hit_info_t {
        int hit_count;      /**< 单词在文本中出现的次数 */
        struct {
                int begin,end;
        }*positions;       /**< 存放那个关键词出现的位置 */
};

/** @see cws_forward_index_t */
typedef struct cws_forward_index_t cws_forward_index_t;

/** 正排索引的结构定义 */
struct cws_forward_index_t {
        unsigned int doc_id;       /**< 一个文档的编号 */
        struct {
                unsigned int word_id;
                cws_hit_info_t *hits;
        };
};

/** @see cws_dict_node_t */
typedef struct cws_dict_node_t cws_dict_node_t;

/** 字典树的一个节点 */
struct cws_dict_node_t {
        unsigned int word_id;    /**< 单词的编号，-1 表示是无效ID，即从第一层到本层的字节还不构成一个单词 */
        cws_dict_node_t *children[CHILDREN_BYTES_COUNT];         /**< 孩子节点 */
};



FirstWord firstWords[GB2312NUM];/* 汉语中词语的首字的集合 */
/* 根据汉字的区位码取得索引值，根据WinHex观察，文件中第一个字节为区号，第二个字节为位号 */
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
* @brief 从字典文件读取并创建字典树
* @author soulmachine@gmail.com
* @param[in] fpDict 字典文件
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
/* 从字典文件读取并创建Trie树 */
/* firstWords表示首字的数组，num表示数组的大小，对于GB2312，num为GB2312NUM */
/**
* @brief 从字典文件读取并创建Trie树
* @author soulmachine@gmail.com
* @param[in] fpDict 字典文件
* @param[in] pFirstWords FirstWord数组首地址
* @return 
* @note 无
* @remarks 无
*/
bool createTrieTree(FILE* fpDict, FirstWord* pFirstWords)
{
        size_t nDictSize = 0; /* 字典文件的大小 */
        size_t readCount = 0; /* fread 返回的实际读取的字节数 */
        unsigned char* pbDictBuffer;/* 存放整个dict文件的缓冲区 */
        size_t i = 0, j = 0; /* 循环计数 */
        FirstWord* currentWord= NULL; /* 正在处理的首字节点 */
        TreeNode** currentNode= NULL; /* 正在处理的树节点 */

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

        readCount = fread(pbDictBuffer, sizeof(unsigned char), nDictSize, fpDict); //读取字典到dict数组
        assert(readCount == nDictSize);

        debug_printf("line %d:\tdictSize = %u\n", nDictSize);

        while (i < nDictSize)
        {
                int index = wordToIndex(pbDictBuffer[i],pbDictBuffer[i+1]);
                if(index < 0)
                {
                        // 忽略这一行的词
                        while(pbDictBuffer[++i] != 0x0d);
                        i+=2;//跳过 0x0a
                        continue;
                }
                else
                {
                        currentWord = &(pFirstWords[index]);
                }

                debug_printf("line %d:\tstart firstWord[%x,%x]\n",__LINE__,pbDictBuffer[i],pbDictBuffer[i+1]);

                (*currentWord).word[0] = pbDictBuffer[i];
                (*currentWord).word[1] = pbDictBuffer[i + 1];
                /* children 在main中初始化了 */
                i += 2; /* 开始处理词语的第三个字节 */
                if(pbDictBuffer[i] != 0x0d)
                {
                        currentNode = &(currentWord->children[pbDictBuffer[i]]);
                        if(*currentNode == NULL)
                        {
                                *currentNode = (TreeNode*)malloc(sizeof(TreeNode)); /*指针变为非NULL，表示增加了一个孩子 */
                                assert(*currentNode != NULL);
                                /* 初始化 */
                                (*currentNode)->occurence = 0;
                                for (j = 0; j < 256; ++j)
                                {
                                        (*currentNode)->children[j] = NULL; /* 表示还没有后续字节 */
                                }

                        }

                        debug_printf("line %d:\tfirstWord[%x,%x]->children[%x] = %x\n",__LINE__,pbDictBuffer[i - 2],pbDictBuffer[i - 1],pbDictBuffer[i],*currentNode);

                        ++i;
                }
                else
                {/* 此时后面必定是个0x0a，跳过，进行新的循环 */
                        i += 2;
                        continue;
                }
                /* 开始处理第四个字节以及后续字节 */
                while (0x0d != pbDictBuffer[i])
                {
                        /* 添加一个子节点 */
                        currentNode = &((*currentNode)->children[pbDictBuffer[i]]);/* 在父节点找到对应的位置 */
                        if(NULL == *currentNode)
                        {
                                *currentNode = (TreeNode*)malloc(sizeof(TreeNode)); /*指针变为非NULL，表示增加了一个孩子 */
                                assert(*currentNode != NULL);
                                /* 初始化 */
                                (*currentNode)->occurence = 0;
                                for (j = 0; j < 256; ++j)
                                {
                                        (*currentNode)->children[j] = NULL; /* 表示还没有后续字节 */
                                }
                        }

                        debug_printf("line %d:\tchildren[%x]  = %x\n",__LINE__,pbDictBuffer[i],*currentNode);

                        ++i;
                }
                /* 此时后面必定是个0x0a，进行新的循环 */
                i += 2;
        };

        free(pbDictBuffer);
        pbDictBuffer = NULL;

        debug_printf("leave function createTrieTree\n");
malloc_failed:
        return FALSE;

}
/* 前序遍历并删除各个节点，打印结果 */
void preTraverse(FILE* const resultFile,const TreeNode** const node,unsigned char indexOfNode,const unsigned char* const firstTwoBytes)
{
        static unsigned char stack[MAXLENGTHOFWORD];/*当成栈来用 */
        static int top = 0; /* 栈顶*/
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

                /* 判断是不是叶子节点*/
                for (i = 0; i < 256; ++i)
                {
                        if (NULL != (*node)->children[i])
                        {
                                break;
                        }
                }
                if (256 == i)
                { /*是叶子节点 */
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
/* 遍历每个字所代表的树 */
void traverseTrees(FILE* const resultFile,const FirstWord* const firstWords)
{
        size_t i = 0, j = 0;
        for (i = 0; i < GB2312NUM; ++i)
        {
                for (j = 0; j < 256; ++j)
                        preTraverse(resultFile,(const TreeNode **const)&(firstWords[i].children[j]),j,firstWords[i].word);
        }
}

/* 检查是否是汉字的第一个字节 */
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

/* 检查是否是汉字的第二个字节 */
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
/* 判断两个字节是不是构成一个汉字 */
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
/* 判断是否是单字*/
BOOL isSingle(unsigned char firstByte,unsigned char secondByte,unsigned char followedByte){
        return(NULL == firstWords[wordToIndex(firstByte,secondByte)].children[followedByte]);
}

/* 检查是否是英文字母 */
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

/* 其他的为特殊符号，断句用，之后就丢弃掉 */
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
/* 在中文串中搜索第一个词语 ，返回值的表示第一个词语的字节数  */
/* start指向的字节必须是一个词语的开始，end指向的字节是词语的结尾的后一个字节*/
/* 由segmentation 保证这点 */
/* start  由 segmentation 传递过来，总是单字或词语的首字，把start 和 end 指向的字节流当成一个环形数组
*  返回值表示匹配到的词语的最长字节数，至少为4
*/
unsigned int searchFirstWord(const unsigned char* start, const unsigned char* const end)
{
        TreeNode *node;
        const unsigned char* index = start;
        size_t i = 0;/* 禁用用于330 行中的for 循环*/
        assert(isAChinese(*start,*(start + 1)));

        debug_printf("in function search\n");
        debug_printf("line %d:\tstart firstWord[%x,%x]\n", __LINE__,*start,*(start + 1));

        node = firstWords[wordToIndex(*start,*(start + 1))].children[*(start + 2)];
        assert(NULL != node);

        debug_printf("lint %d:\tfirstWord[%x,%x].children[%x] = %x\n",__LINE__,*start,*(start + 1),*(start + 2),node);


        for (index = start + 3; index < end; index += 2)
        {/*node 指向汉字的第一个字节，i指向汉字的第二个字节 */

                assert(isAChinese(*(index-1),*index));

                debug_printf("line %d:\t%x %x\n",__LINE__,*index,*(index+1));

                if (node != NULL)
                {
                        debug_printf("line %d:\t%x->children[%x] = %x\n",__LINE__,node,*index,node->children[*index]);

                        node = node->children[*index]; /* 下移一层 */


                        if (node != NULL)
                        {
                                debug_printf("line %d:\t%x->children[%x] = %x\n",__LINE__,node,*(index + 1),node->children[*(index + 1)]);

                                /* 判断node 是否是叶子节点 */
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
                        {/*偶数字节是在Trie树中，奇数字节却不在(从0开始)，也就是说一个汉字只有"一半"在Trie 树中，不可能发生 */
                                /* 不可能执行到此处 */
                                fprintf(stderr,"fatal error in trie!\n");
                                exit(-1);
                        }
                }
        }

        debug_printf("leave function search\n");

        assert(1 == index - end); /* 此时index 肯定在end后面*/
        return index - start - 1;
}


/* 对一个合法的词语进行穷举匹配，从词语的第二个汉字到倒数第二个汉字，
* 因为它们都有可能是一个词语的开头
* startOfWord表示一个词语的第一个字节，endOfWord 指向词语最后一个字节的下一字节
*/
void wordOfWord(const unsigned char* startOfWord, const unsigned char* const endOfWord)
{
        for(startOfWord += 2; startOfWord < endOfWord - 4;startOfWord += 2)
        {
                (void)searchFirstWord(startOfWord,endOfWord);
        }
}
void segmentation(FILE* const resultFile,FILE* const englishFile,FILE* textFile)/* 中文分词主程序 */
{
        /* 为了能识别"中华人民共和国"中的"人民"，"人民共和国"，"共和国"，需要额外设置一个embededStart指针 */
        /* unsigned char* start,end,embededStart; */
        unsigned char* buffer;/*fread 的缓冲区 */
        unsigned int length = 0; /* 每次匹配到的词语的长度 */
        size_t textFileOffset = 0;
        size_t bufferStart = 0; /* 仅用于输出英文*/
        size_t bufferEnd = 0;
        size_t chineseBufferStart = 0; /* buffer + chineseBufferStart 和 */
        size_t chineseBufferEnd = 0; /* buffer + chineseBufferEnd 之间的字节流会传递给searchFirstWord */
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
                        BUFFERSIZE/*要确保缓冲区能装下一个完整的词语，必须考虑极端情况：缓冲区首尾都是0x0d0a */,
                        textFile)) > 0)
                {/* 主要逻辑在这里面 */
                        /*读入一次缓冲取，利用非汉字，即英文和特殊符号断句 */

                        debug_printf("line %d:\t textFileOffset = %d\r\n",__LINE__,textFileOffset);

                        while(bufferEnd < textFileOffset - 1) /* buffer 最后一个字节作为哨兵 */
                        {
                                length = 0;

                                debug_printf("line %d:\t start while with:%x,%x\r\n",__LINE__,*(buffer + bufferEnd),*(buffer + bufferEnd + 1));

                                while(isSymbol(*(buffer + bufferEnd)))
                                {/* 忽略符号*/
                                        ++bufferEnd;
                                }
                                bufferStart = bufferEnd;

                                while(isEnglish(*(buffer + bufferEnd)))
                                {
                                        ++bufferEnd;
                                }
                                fwrite(buffer + bufferStart,sizeof(unsigned char),bufferEnd - bufferStart,englishFile);
                                bufferStart = bufferEnd;

                                /* 忽略前导单字以及乱码*/
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
                                        else/* 第二字节是乱码*/
                                        {
                                                ++bufferEnd;/* 跳过*/
                                        }
                                }

                                while(firstIsWord(*(buffer + bufferEnd))){
                                        ++bufferEnd;
                                        if(secondIsWord(*(buffer + bufferEnd)))
                                        {
                                                ++bufferEnd;
                                        }
                                        else/* 第二字节是乱码*/
                                        {
                                                hasSecondConfused = TRUE;
                                                ++bufferEnd;/* 跳过*/
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

