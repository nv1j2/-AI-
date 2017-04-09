#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <string.h>

#define gotoxyPoolWidth 12      //游戏池左侧宽度
#define poolWidth 14           //游戏池宽度 包括左右边框各1,实际宽度为12
#define poolHidth 27           //游戏池高度 包括下边框1和上部分方块显示区域x4,实际高度为22

HANDLE  hConsole;// 控制台输出句柄

static const bool tetrisBox[7][8]={  //存储方块形状
	{1,1,1,1,
	 0,0,0,0},// I型方块
	{1,1,1,0,
	 0,1,0,0},// T型方块
	{1,1,0,0,
	 1,1,0,0}, // O型方块
    {1,1,1,0,
	 1,0,0,0}, // L型方块
	{1,1,1,0,
	 0,0,1,0},// J型方块
	{1,1,0,0,
     0,1,1,0},// Z型方块
	{0,1,1,0,
	 1,1,0,0} // S型方块
};

typedef struct{
	int eliminateLineNum[4];//消除行数统计
	int tetrisBoxType[7];//方块类型统计
	int gameGrade;//总成绩
}gameData;

typedef struct gameMainData{//这里本来是有记录排名的,但是由于存储的
	char name[20];              //名称
	gameData one;            //其他信息
	struct gameMainData *next,*prior;
}gameMainData;

typedef struct gameManager{
	bool pool[4][4];   //游戏池,1为方块
	bool savePool[poolWidth][poolHidth];  //存储游戏池,1为方块
	bool gameDead;   //游戏死亡判断,0为死亡
	int NowboxX;//方块横坐标,左上坐标,相对于游戏池
	int NowboxY;//方块竖坐标
	int NowboxShape;//当前方块形状
	int SboxShape;//上个方块形状
	int SSboxShape;//上上个方块形状
} gameManager;

typedef struct autoPlayWay{//AI摆法
    bool pool[4][4];//游戏池
    bool saveMove[poolWidth][poolHidth];//记录AI走过的位置,只有旋转过才算走过
    int rotateFrequency;//旋转次数，不同方块旋转次数不同
    int moveWay[100];//AI操作路径;72(上)为旋转,75(左)为左移,77(右)为右移,80(下)为下移,32(空格)为直接落地,最大操作数为100,0终止;PS:这里其实使用栈更好,不过麻,这里数据很简单,就懒得创建栈了。
    int boxX;//记录原来的X坐标
    int saveMoveX[poolHidth-1];//记录移动Y时的X坐标,作用相当于栈
    int boxY;//记录原来的Y坐标
    int value;//估值
    int priority;//优先级
}autoPlayWay;

void gotoxy(int x,int y,int z);              //改变光标位置函数
void printPlayBorder();  // 显示游戏池边界
void startGameInfo(gameManager *manager,gameMainData *mainData); //初始化游戏信息
int  rollOneNum(int num);  //随机一个数，包括0
void startRunBoxGame(gameManager *manager,gameMainData *mainData); //游戏更新开始信息
void printRunBoxGame(gameManager *manager,int printColor); //显示游戏运行时方块
void removePoolTetris(gameManager *manager);    //移除游戏池当前方块显示
void rotatePoolTetris(gameManager *manager,bool v,bool u);   //旋转游戏池方块;bool,v为1进行碰撞检测且顺时针旋转,v为0不进行碰撞检测且逆时针旋转
bool checkCollision(gameManager *manager,bool v); // 旋转碰撞检测,返回1为碰撞,0为没碰撞,后面bool:1为旋转,bool:0为非旋转
void keyControl(gameManager *manager,int key,gameMainData *mainData);//按键
void dropDownTetris(gameManager *manager,gameMainData *mainData);//方块落地
void tetrisBoxGameDead(gameManager *manager);//游戏死亡判断
void eliminateLine(gameManager *manager,gameMainData *mainData); //消除行
void printGameInfo(gameManager *manager,gameMainData *mainData);//显示游戏必要信息
void scanfPlayerName(gameMainData *mainData);//输入游戏玩家名
void addGameDataSave(gameMainData *mainData);//存储游戏信息到文件
void readListWithFile(gameMainData *mainData);//从文件中读取数据，并保存在双向链表中
void destroyList(gameMainData *mainData);//删除除了头节点外的所有节点
void quick_sort(gameMainData *mainData,gameMainData *lowList,gameMainData *highist);//链表快速排序组件
gameMainData *partion(gameMainData *mainData,gameMainData *lowList,gameMainData *highist);//链表快速排序组件
void runTetrisGame(gameManager *manager,gameMainData *mainData);//运行游戏
int mainGameMenu();//游戏主菜单
void mainGameMenuChoice(gameManager *manager,gameMainData *mainData);//游戏菜单选择
HWND GetConsoleHwnd(void);//获取控制台窗口句柄
void drawBorder();//GDI绘图绘边框
void drawELSFK();//画出俄罗斯方块的星阵
void gameHelp();//游戏帮助说明
void printfGameData(gameMainData *mainData);//输出显示数据排名
void autoPlay(gameManager *manager,gameMainData *mainData);//挂载AI,自动运行
int evaluate(gameManager *manager);//AI;估值
int landingHeight(gameManager *manager);//AI;下落高度
int eliminateRows(gameManager *manager);//AI;消行数*当前落子被消取的格子数
int boardRowTransitions(gameManager *manager);//AI;行变换
int boardColTransitions(gameManager *manager);//AI;列变换
int buriehHoles(gameManager *manager);//AI;空洞
int wells(gameManager *manager);//AI;井
void existencePlace(gameManager *manager,autoPlayWay *bestPlayWay);//AI;方块的存在位置
bool routing(gameManager *manager,autoPlayWay *nowPlayWay,int v);//AI;寻径,非最佳路径,只要能够到达就行;深度递归;类似于二叉树中序遍历;v为递归层次;
bool pooltest(gameManager *manager,autoPlayWay *nowPlayWay);//AI;游戏池检测;游戏池检测,相同返回0,不同返回1;
void changeMoveWay(const gameManager *manager,autoPlayWay *bestPlayWay);//处理改变移动路径

void easyAI(gameManager *manager,gameMainData *mainData);//临时简单AI
void easyExistencePlace(gameManager *manager,autoPlayWay *bestPlayWay);//简单AI寻址

/*或许有人会说我这个代码格式看起来有些不大众规范，代码比较紧密，不过我觉得这样更容易看出代码之间的规律，有利于我个人的效率，而且我也是练练手*/
void startGameInfo(gameManager *manager,gameMainData *mainData){  //初始化游戏信息
	manager->SboxShape=rollOneNum(14)/2;//上个方块形状      不这样会使得两个随机数总是相同
	manager->SSboxShape=rollOneNum(21)/3;//上上个方块形状
	manager->gameDead=1;//总成绩游戏存活状态
	memset(manager->pool,0,sizeof(manager->pool));//初始化游戏池
	memset(manager->savePool,0,sizeof(manager->savePool));//初始化存储游戏池
	memset(mainData->one.eliminateLineNum,0,sizeof(mainData->one.eliminateLineNum));//初始化消除行数
	memset(mainData->one.tetrisBoxType,0,sizeof(mainData->one.tetrisBoxType));//初始化方块类型
	mainData->one.gameGrade=0;//初始化总成绩
	mainData->next=mainData->prior=NULL;//初始化链表
	for(int i=0;i<poolWidth;i++)    //给存储游戏池边界赋值
		manager->savePool[i][poolHidth-1]=1;
	for(int j=0;j<poolHidth;j++){
		manager->savePool[0][j]=1;
		manager->savePool[poolWidth-1][j]=1;
	}
}
void printPlayBorder(){// 显示游戏池边界
	int i=gotoxyPoolWidth,j=0;
	SetConsoleTextAttribute(hConsole, 0xf); //设置颜色
	for(;j<poolHidth;j++){          //竖线
		gotoxy(gotoxyPoolWidth,j,2);
		printf("■");
		gotoxy(gotoxyPoolWidth+poolWidth-1,j,2);
		printf("■");
	}
	for(;i<gotoxyPoolWidth+poolWidth;i++){        //横线
		gotoxy(i,poolHidth-1,2);
		printf("■");
	}
	for(i=gotoxyPoolWidth+poolWidth;i<gotoxyPoolWidth+poolWidth+13;i++){      //右侧横线
		gotoxy(i,0,2);
		printf("■");
		gotoxy(i,6,2);
		printf("■");
	}
	for(j=1;j<6;j++){       //右侧竖线
	    gotoxy(gotoxyPoolWidth+poolWidth+6,j,2);
		printf("■");
	    gotoxy(gotoxyPoolWidth+poolWidth+12,j,2);
		printf("■");
	}
	SetConsoleTextAttribute(hConsole, 0x0c); //设置颜色
		gotoxy(gotoxyPoolWidth,4,2);
		printf("■");
		gotoxy(gotoxyPoolWidth+poolWidth-1,4,2);
		printf("■");
	SetConsoleTextAttribute(hConsole, 0xf0); //设置颜色
		gotoxy(2,1,1);
		printf("消除1行数:0\n  ");
		printf("消除2行数:0\n  ");
		printf("消除3行数:0\n  ");
		printf("消除4行数:0\n\n\n\n  ");
		printf("I型方块数:0\n  ");
		printf("T型方块数:0\n  ");
		printf("O型方块数:0\n  ");
		printf("L型方块数:0\n  ");
		printf("J型方块数:0\n  ");
		printf("Z型方块数:0\n  ");
		printf("S型方块数:0\n\n\n\n  ");
		printf(" 总成绩:0\n");
}
int rollOneNum(int num){//随机一个数，包括0
	srand((unsigned)time(NULL));
	return rand()%num;
}
void gotoxy(int x,int y,int z){//x为列坐标,y为行坐标,z为与x相乘（因为"■"字符占两位），一般z值为1或2
	COORD pos={z*x,y};
	HANDLE hOut=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hOut,pos);
}
void startRunBoxGame(gameManager *manager,gameMainData *mainData){//游戏更新开始信息
	int i,j;
	tetrisBoxGameDead(manager);//游戏死亡判断
	SetConsoleTextAttribute(hConsole,0xff); //设置颜色
	for(i=0;i<4;i++)for(j=0;j<2;j++){gotoxy(gotoxyPoolWidth+poolWidth+8+i,2+j,2);printf("■");}//将上上个方块区域清空
	for(i=0;i<4;i++)for(j=0;j<2;j++){gotoxy(gotoxyPoolWidth+poolWidth+2+i,2+j,2);printf("■");}//将上个方块区域清空
	memset(manager->pool,0,sizeof(manager->pool));//初始化游戏池
	manager->NowboxShape=manager->SboxShape;//将上个方块形状给当前方块
	manager->SboxShape=manager->SSboxShape;//将上上个方块形状给上个方块
	manager->SSboxShape=rollOneNum(7);//上上个方块形状
	mainData->one.tetrisBoxType[manager->NowboxShape]++;//方块类型统计
	manager->NowboxX=6;//当前方块出现的横坐标
	manager->NowboxY=0;//当前方块出现的竖坐标
	for(i=0;i<4;i++) //将当前方块加入游戏池
		for(j=0;j<2;j++)
			manager->pool[i][j]=tetrisBox[manager->NowboxShape][i+(j*4)];
	printGameInfo(manager,mainData);//显示游戏统计信息
	printRunBoxGame(manager,manager->NowboxShape+1);//显示游戏运行时方块
}
void printGameInfo(gameManager *manager,gameMainData *mainData){ //显示游戏统计信息
	int i,j;
	SetConsoleTextAttribute(hConsole,241+manager->SboxShape); //设置颜色
	for(i=0;i<4;i++)
		for(j=0;j<2;j++)
			if(tetrisBox[manager->SboxShape][i+(j*4)]==1){//显示上个方块
			gotoxy(gotoxyPoolWidth+poolWidth+2+i,2+j,2);
	     	printf("■");
			}	
	SetConsoleTextAttribute(hConsole,241+manager->SSboxShape); //设置颜色
	for(i=0;i<4;i++)
		for(j=0;j<2;j++)
			if(tetrisBox[manager->SSboxShape][i+(j*4)]==1){//显示上上个方块
				gotoxy(gotoxyPoolWidth+poolWidth+8+i,2+j,2);
				printf("■");
			}
	SetConsoleTextAttribute(hConsole,0xf0); //设置颜色
	gotoxy(12,manager->NowboxShape+8,1); //显示方块类型统计
	printf("%d",mainData->one.tetrisBoxType[manager->NowboxShape]);
	for(i=0;i<4;i++){//显示消除行数统计
		gotoxy(12,i+1,1);
		printf("%d",mainData->one.eliminateLineNum[i]);
	}
	gotoxy(10,18,1);//显示统计总成绩
	printf("%d",mainData->one.gameGrade);
}
void printRunBoxGame(gameManager *manager,int printColor){       //显示游戏运行时方块
	SetConsoleTextAttribute(hConsole,240+printColor); //设置颜色
	for(int i=0;i<4;i++)
		for(int j=0;j<4;j++)
			if(manager->pool[i][j]==1){
				gotoxy(gotoxyPoolWidth+i+manager->NowboxX,j+manager->NowboxY,2);
				printf("■");
			}
}
void removePoolTetris(gameManager *manager){    //移除游戏池当前方块显示
	SetConsoleTextAttribute(hConsole,0xff); //设置颜色
	for(int i=0;i<4;i++)
		for(int j=0;j<4;j++)
			if(manager->pool[i][j]==1){
				gotoxy(gotoxyPoolWidth+i+manager->NowboxX,j+manager->NowboxY,2);
				printf("■");
			}
}
void rotatePoolTetris(gameManager *manager,bool v,bool u){   //旋转游戏池方块;bool,v为1进行碰撞检测且顺时针旋转,v为0不进行碰撞检测且逆时针旋转
	bool flashSaveBox[4][4];       //临时存储方块形状
	int i,j;
	for(i=0;i<4;i++)//保存原方块形状
		for(j=0;j<4;j++)
			flashSaveBox[i][j]=manager->pool[i][j];
	memset(manager->pool,0,sizeof(manager->pool));//初始化游戏池
	
	if(u){
		for(i=0;i<4;i++)//将方块旋转 顺时针
			for(j=0;j<4;j++)
	          manager->pool[j][3-i]=flashSaveBox[i][j];//将方块旋转 顺时针
	}
    else{
		for(i=0;i<4;i++)//将方块旋转 顺时针
			for(j=0;j<4;j++)
				manager->pool[3-j][i]=flashSaveBox[i][j];//将方块旋转 逆时针
		for(int m=0;m<3;m++)//进行右对其
			if(manager->pool[0][0]||manager->pool[0][1]||manager->pool[0][2]||manager->pool[0][3])
				break;
			else
				for(j=0;j<4;j++){
					for(i=0;i<3;i++)
						manager->pool[i][j]=manager->pool[i+1][j];
					manager->pool[3][j]=0;
				}
	}
	for(int l=0;l<3;l++)//进行上对其
		if(manager->pool[0][0]||manager->pool[1][0]||manager->pool[2][0]||manager->pool[3][0])
			break;
		else
			for(i=0;i<4;i++){
				for(j=1;j<4;j++)
					manager->pool[i][j-1]=manager->pool[i][j];
					manager->pool[i][3]=0;
			}
	if(v){
        if(checkCollision(manager,1)){// 旋转碰撞检测,返回1为碰撞,0为没碰撞,后面bool:1为旋转,bool:0为非旋转
            for(i=0;i<4;i++)//还原原方块形状
                for(j=0;j<4;j++)
                    manager->pool[i][j]=flashSaveBox[i][j];
        }
    }
}
bool checkCollision(gameManager *manager,bool v){ // 旋转碰撞检测,返回1为碰撞,0为没碰撞,后面bool:1为旋转,bool:0为非旋转
	bool u=1;
	for(int i=0;i<4;i++){
		for(int j=0;j<4;j++)
			if(manager->pool[i][j]&&manager->savePool[manager->NowboxX+i][manager->NowboxY+j]){
				if(v){
					for(int l=1;l<4;l++){//防止在右侧有遮挡时无法旋转
						u=1;
						for(i=0;i<4;i++){
 							if(manager->NowboxX+i-l>=poolWidth)break;
							for(j=0;j<4;j++)
								if(manager->pool[i][j]&&manager->savePool[manager->NowboxX+i-l][manager->NowboxY+j])
									u=0;	
						}
						if(u){
							manager->NowboxX=manager->NowboxX-l;
							return 0;
						}	
					}
					return 1;
				}
				else 
					return 1;
			}
	}
	return 0;
}
void keyControl(gameManager *manager,int key,gameMainData *mainData){// 按键
	removePoolTetris(manager);//移除游戏池当前方块显示
    switch(key){
		case 72: rotatePoolTetris(manager,1,1);break; //上键，旋转游戏池方块
		case 75: --manager->NowboxX;if(checkCollision(manager,0))++manager->NowboxX;break;  // 左键，水平左移动方块
		case 77: ++manager->NowboxX;if(checkCollision(manager,0))--manager->NowboxX;break;  // 右键，水平右移动方块
		case 80: ++manager->NowboxY;if(checkCollision(manager,0)){--manager->NowboxY;dropDownTetris(manager,mainData);}break;		// 下键	
		case 32: while(1){++manager->NowboxY;if(checkCollision(manager,0)){--manager->NowboxY;dropDownTetris(manager,mainData);break;}}break;//空格键
		default:break;
    }
	printRunBoxGame(manager,manager->NowboxShape+1);
}
void dropDownTetris(gameManager *manager,gameMainData *mainData){//方块落地
	for(int i=0;i<4;i++)//将游戏池存储
		for(int j=0;j<4;j++)
			if(manager->pool[i][j])
				manager->savePool[manager->NowboxX+i][manager->NowboxY+j]=manager->pool[i][j];
	printRunBoxGame(manager,0);//显示游戏运行时方块
	eliminateLine(manager,mainData);//消除行
	startRunBoxGame(manager,mainData);//游戏更新开始信息
}
void tetrisBoxGameDead(gameManager *manager){  //游戏死亡判断
	for(int i=1;i<poolWidth-1;i++)
		if(manager->savePool[i][3]){
			manager->gameDead=0;
			break;
		}
}
void eliminateLine(gameManager *manager,gameMainData *mainData){ //消除行
	bool v;
	int L=0;//统计消除行数
	for(int i=0;i<4;i++){
		v=1;
		if(i+manager->NowboxY+1==poolHidth)break;//防止超出边界
		for(int j=1;j<poolWidth-1;j++)//判断是否有消除
			if(!manager->savePool[j][manager->NowboxY+i])
				v=0;
		if(v){//如果有,移动刷新 存储游戏池
			L++;
			for(int m=1;m<poolWidth-1;m++)
				for(int n=manager->NowboxY+i;n>1;n--){
					manager->savePool[m][n]=manager->savePool[m][n-1];
					gotoxy(m+gotoxyPoolWidth,n,2);
					if(manager->savePool[m][n]){
						SetConsoleTextAttribute(hConsole,0xf0); //设置颜色
						printf("■");
					}
					else{
						SetConsoleTextAttribute(hConsole,0xff); //设置颜色
						printf("■");
					}
				}
		}
	}
	if(L){
		mainData->one.eliminateLineNum[--L]++;//统计消除行数
		mainData->one.gameGrade=mainData->one.gameGrade+L*(++L);//统计总成绩
	}
}
void addGameDataSave(gameMainData *mainData){    //存储游戏信息到文件
	FILE *fp;
	int i;
	fp=fopen("TetrisGameData.dat","a");
	for(i=0;i<4;i++)
		fprintf(fp,"%d ",mainData->one.eliminateLineNum[i]);
	for(i=0;i<7;i++)
		fprintf(fp,"%d ",mainData->one.tetrisBoxType[i]);
	fprintf(fp,"%d ",mainData->one.gameGrade);
	fprintf(fp,"%s\n",mainData->name);
	fclose(fp);
}
void readListWithFile(gameMainData *mainData){//从文件中读取数据，并保存在双向链表中
	FILE *fp;
	gameMainData *p1, *p2;
	int count;
	fp=fopen("TetrisGameData.dat","r");
	if(fp==NULL){
		fp=fopen("TetrisGameData.dat","w");
		fclose(fp);
		return;
	}
	fseek(fp,0L,2);
	count=ftell(fp);
	p1=mainData;
	fp=fopen("TetrisGameData.dat","r");
	while(!feof(fp)){
		p2=(gameMainData *)malloc(sizeof(gameMainData));
		fscanf(fp,"%d %d %d %d %d %d %d %d %d %d %d %d %s\n",&p2->one.eliminateLineNum[0],&p2->one.eliminateLineNum[1],
			&p2->one.eliminateLineNum[2],&p2->one.eliminateLineNum[3],
			&p2->one.tetrisBoxType[0],&p2->one.tetrisBoxType[1],&p2->one.tetrisBoxType[2],&p2->one.tetrisBoxType[3],
			&p2->one.tetrisBoxType[4],&p2->one.tetrisBoxType[5],&p2->one.tetrisBoxType[6],
			&p2->one.gameGrade,&p2->name);
		p1->next=p2;
		p2->prior=p1;
		p1=p2;
		if(ftell(fp)==count)
			break;
	}
	p2->next=NULL;
	fclose(fp);
	quick_sort(mainData,mainData->next,p2);//快速排序(头节点,高位节点,尾节点)
}
gameMainData *partion(gameMainData *mainData,gameMainData *lowList,gameMainData *highist){//链表快速排序组件
	char nameTemp[20];
	strcpy(nameTemp,lowList->name);
	gameData dataTemp=lowList->one;
	while(lowList!=highist){
		while(lowList!=highist&&highist->one.gameGrade<=lowList->one.gameGrade)highist=highist->prior;
		strcpy(nameTemp,lowList->name);strcpy(lowList->name,highist->name);strcpy(highist->name,nameTemp);//交换姓名
		dataTemp=lowList->one;lowList->one=highist->one;highist->one=dataTemp;//交换其他详细数据
	  /*if(lowList==highist)return Ret->next;//。以注释形式保留
		lowList->prior->next=highist;//因为头节点不参与排序，所以lowList的prior指针不会指向NULL
		if(highist->next)highist->next->prior=lowList;//防止超出链表尾端
		if(lowList->next==highist){
			highist->prior=lowList->prior;
			lowList->next=highist->next;
			lowList->prior=highist;
			highist->next=lowList;
		}
		else{
			highist->prior->next=lowList;
			if(lowList->next)lowList->next->prior=highist;//防止超出链表尾端
			temp=lowList->prior;lowList->prior=highist->prior;highist->prior=temp;
			temp=lowList->next;lowList->next=highist->next;highist->next=temp;
		}
		temp=lowList;lowList=highist;highist=temp;//将两节点位置还原*/
		while(lowList!=highist&&lowList->one.gameGrade>=lowList->one.gameGrade)lowList=lowList->next;
		strcpy(nameTemp,lowList->name);strcpy(lowList->name,highist->name);strcpy(highist->name,nameTemp);//交换姓名
		dataTemp=lowList->one;lowList->one=highist->one;highist->one=dataTemp;//交换其他详细数据
	}
	return lowList;  
}   
void quick_sort(gameMainData *mainData,gameMainData *lowList,gameMainData *highist){//链表快速排序组件
	gameMainData *quickSortTemp=NULL;
	quickSortTemp=partion(mainData, lowList, highist);  //枢轴
	if(lowList!=quickSortTemp)
		quick_sort(mainData,lowList,quickSortTemp->prior);
	if(highist!=quickSortTemp)
		quick_sort(mainData,quickSortTemp->next,highist);  
}
void printfGameData(gameMainData *mainData){ //输出显示数据排名
	int i=0,pLink=0;
	readListWithFile(mainData); //从文件中读取数据，并保存在链表中
	if(!mainData->next){//防止链表建立不成功
		printf("\t\t目前并没有数据用于排行");
		getch();
		return;
	}
	gameMainData *p1=mainData->next;
	SetConsoleTextAttribute(hConsole,0xf0); //设置颜色
	printf("\n 排名  玩家名  总分  消1行 消2行 消3行 消4行  I型  T型  O型  L型  J型  Z型  S型\n");
	do{ pLink++;
		printf("  %-3d  ",pLink);
		printf("%-8s",p1->name);
		printf("%4d ",p1->one.gameGrade);
		for(i=0;i<4;i++)
			printf("%5d ",p1->one.eliminateLineNum[i]);
		for(i=0;i<7;i++)
			printf("%4d ",p1->one.tetrisBoxType[i]);
		printf("\n");
		p1=p1->next;
		if(pLink>9)break;//只显示前10名
	}while(!(p1==NULL));
	destroyList(mainData);//删除除了头节点外的所有节点
	HWND hWnd=GetConsoleHwnd();//获取控制台窗口句柄，用于设置画笔颜色
	HDC hDC=GetDC(hWnd);
	HPEN hNewPen=CreatePen(PS_SOLID,1,RGB(0,255,255));//创建画笔对象
	HPEN hOldPen=(HPEN)SelectObject(hDC,hNewPen);//选取画笔对象进行绘图
	CONSOLE_SCREEN_BUFFER_INFO bInfo; // 窗口信息
	HANDLE Hout=GetStdHandle(STD_OUTPUT_HANDLE);//获取控制台句柄
	GetConsoleScreenBufferInfo(Hout, &bInfo );//获取控制台信息
	SetBkMode(hDC, TRANSPARENT);SetTextColor(hDC,255*255);/*这里应该也有更好的处理办法，这一大段均重复代码*/
	for(i=1;i<pLink+3;i++){
		MoveToEx(hDC,5,i*16,NULL);LineTo(hDC,635,i*16);}
	MoveToEx(hDC,5,16,NULL);LineTo(hDC,5,(pLink+2)*16);
	MoveToEx(hDC,635,16,NULL);LineTo(hDC,635,(pLink+2)*16);

	getch();
	InvalidateRect(hWnd,NULL,FALSE);//清除GDI画图画出全部线条
}
void destroyList(gameMainData *mainData){//删除除了头节点外的所有节点
	gameMainData *temp=mainData;
	if(temp){
		temp=mainData->next;
		mainData=temp;
		free(temp);
	}
}
void scanfPlayerName(gameMainData *mainData){//输入游戏玩家名
	system("cls");
	SetConsoleTextAttribute(hConsole,0xf0); //设置颜色
	gotoxy(3,8,1);
	printf("你的总成绩为%4d ",mainData->one.gameGrade);
	gotoxy(29,2,1);
	printf("请输入你的名字");
	gotoxy(25,3,1);
	printf("┏━━━━━━━━━━┓");
	gotoxy(25,4,1);
	printf("┃                    ┃");
	gotoxy(25,5,1);
	printf("┗━━━━━━━━━━┛");
	gotoxy(28,4,1);
	scanf("%s",&mainData->name);
	system("cls");
}
void runTetrisGame(gameManager *manager,gameMainData *mainData){//运行游戏
	int ch;
	clock_t clockLast, clockNow;
	startGameInfo(manager,mainData);//初始化游戏框架
	printPlayBorder();// 显示游戏池边界
	startRunBoxGame(manager,mainData);//游戏更新开始信息
    clockLast=clock();  // 计时
	while(manager->gameDead){  // 判断是否存活
        while(_kbhit()){ // 有键按下
			ch=_getch();
			if(ch==27){//esc键,暂停
				clockLast=0;
				SetConsoleTextAttribute(hConsole,0x9f);
				gotoxy(28,27,1);
				printf("游戏暂停,按任意键继续");
				system("pause > nul");
				SetConsoleTextAttribute(hConsole,0xf0);
				gotoxy(28,27,1);
				printf("                      ");
			}
			if(ch=='Z'||ch=='z')return;//返回退出
			if(ch=='X'||ch=='x')autoPlay(manager,mainData);    //挂载AI
			if(ch=='C'||ch=='c')easyAI(manager,mainData);//临时简单AI
			keyControl(manager,ch,mainData); // 按键
		}
	clockNow=clock();  // 计时
		if(clockNow-clockLast>0.5F*CLOCKS_PER_SEC){// 两次记时的间隔超过0.5秒
			clockLast=clockNow;
				keyControl(manager,80,mainData);  // 方块往下移
			}
	}
	scanfPlayerName(mainData);//初始化输入游戏玩家名
	addGameDataSave(mainData);//存储游戏信息到文件,先存再读,这样原mainData作为头节点,数据将复制到链表结尾(未排序前)
}


int landingHeight(gameManager *manager){//AI;下落高度
    float height;
    height=(float)(poolHidth-manager->NowboxY-1);
    for(int i=3;i>=0;i--){
        for(int j=0;j<4;j++)
            if(manager->pool[i][j])
                return (int)((height-((float)i/2))*45);
  }
  return 0;//没有什么作用，减少警告
}
int eliminateRows(gameManager *manager){//AI;消行数*当前落子被消取的格子数
    int eliNum=0;//消行数
    int boxNum=0;//当前落子被消取的格子数
    bool v;
    for(int i=0;i<4;i++)//将游戏池存储
		for(int j=0;j<4;j++)
			if(manager->pool[i][j])
							manager->savePool[manager->NowboxX+i][manager->NowboxY+j]=manager->pool[i][j];
    for(int m=0;m<4;m++){
    v=1;
    if(m+manager->NowboxY+1==poolHidth)break;//防止超出边界
    for(int n=1;n<poolWidth-1;n++)//判断是否有消除
		if(!manager->savePool[n][manager->NowboxY+m])
			v=0;
	if(v){//如果有消除
        ++eliNum;
		for(int p=1;p<poolWidth-1;p++)
			for(int q=manager->NowboxY+i;q>1;q--)
				manager->savePool[p][q]=manager->savePool[p][q-1];
		for(int u=0;u<4;u++)
            if(manager->pool[m][u])
                boxNum++;
	}
  }
  return eliNum*boxNum*34;
}
int boardRowTransitions(gameManager *manager){//AI;行变换
    int rowTran=0;
    for(int i=4;i<poolHidth-1;i++)
        for(int j=0;j<poolWidth-1;j++)
            if(manager->savePool[j][i]!=manager->savePool[j+1][i])
                rowTran++;
    return rowTran*32;
}
int boardColTransitions(gameManager *manager){//AI;列变换
    int colTran=0;
    bool v=1;
    for(int i=1;i<poolWidth-1;i++)
        for(int j=4;j<poolHidth;j++)
            if(manager->savePool[i][j]!=v){
                colTran++;
                v=manager->savePool[i][j];
            }
    return colTran*93;
}
int buriehHoles(gameManager *manager){//AI;空洞
    int cavity=0;
    for(int i=1;i<poolWidth-1;i++){
        for(int j=4;j<poolHidth-1;j++)
            if(manager->savePool[i][j])
                break;
        while(j<poolHidth-1){
            if(!manager->savePool[i][j])
                cavity++;
            j++;
        }
    }
    return cavity*79;
}
int wells(gameManager *manager){//AI;井
    int wellNum=0,wellDepth=0;
    static const int wellDepthTable[29]={0, 1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 66, 78,91, 
    105, 120, 136, 153, 171, 190, 210, 231, 253,276, 300, 325, 351, 378, 406};//井的深度权值
    for(int i=1;i<poolWidth-1;i++)
        for(int j=4;j<poolHidth;j++)
            if(!manager->savePool[i][j]){
                if(manager->savePool[i-1][j]&&manager->savePool[i+1][j])
                    wellDepth++;
            }
            else {
                wellNum += wellDepthTable[wellDepth];
                wellDepth=0;
            }
    return wellNum*34;
}
int evaluate(gameManager *manager){//AI;估值
/*使用改进的Pierre Dellacherie,（只考虑当前方块）数据参考:http://tieba.baidu.com/p/2078306985 @wohaaitinciu
	http://ielashi.com/el-tetris-an-improvement-on-pierre-dellacheries-algorithm/
	部分思路参考:湖北大学_工程硕士学位论文_2011_余颖;基于神经网络和遗传算法的人工智能游戏研究与应用;*/
    int value=0;
    value -=landingHeight(manager);  // 下落高度,方块中点距底部的方格数;权值-4.500158825082766
    value +=eliminateRows(manager);//消行数*当前落子被消取的格子数;权值3.4181268101392694
    value -=boardRowTransitions(manager);//行变换;权值-3.2178882868487753
    value -=boardColTransitions(manager);//列变换;权值-9.348695305445199
    value -=buriehHoles(manager);//空洞;权值-7.899265427351652
    value -=wells(manager);//井;权值-3.3855972247263626
  return value;
}

bool pooltest(gameManager *manager,autoPlayWay *nowPlayWay){//AI;游戏池检测;游戏池检测,相同返回1,不同返回0;
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            if(manager->pool[i][j]!=nowPlayWay->pool[i][j])
                return 0;
    return 1;
}
bool routing(gameManager *manager,autoPlayWay *nowPlayWay,int v){//AI;寻径,非最佳路径,只要能够到达就行;从目标坐标到原坐标,以减少递归次数;这里采用原创的流水算法;对每一层进行递归;
    
    nowPlayWay->saveMoveX[manager->NowboxY]=manager->NowboxX;//一旦Y坐标发生变化,记录其对应的X坐标。类似出入栈;目标方块上移
    --manager->NowboxY;//目标方块上移
    ++v;if(v>90)return 0;//对方块移动次数进行判定
    if(((manager->NowboxY>=0))&&(!checkCollision(manager,0))){//碰撞检测,返回1为碰撞,0为没碰撞;
        if(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]){
            nowPlayWay->moveWay[v]=80;//向上
            if((manager->NowboxX==nowPlayWay->boxX)&&(manager->NowboxY==nowPlayWay->boxY)&&(pooltest(manager,nowPlayWay))){nowPlayWay->moveWay[v+1]=0;return 1;}//验证是否达到原位置
            if(routing(manager,nowPlayWay,v))return 1;//递归  
            --v; 
        }
    }
    else{
        ++manager->NowboxY;//目标方块下移
        --v;
        for(int i=0;i<nowPlayWay->rotateFrequency;i++){
            rotatePoolTetris(manager,0,0);//旋转,不进行旋转碰撞检测;放止manager->NowboxX的值被改变
            ++v;if(v>90)return 0;//对方块移动次数进行判定
            if(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]){//碰撞检测,返回1为碰撞,0为没碰撞
                nowPlayWay->moveWay[v]=9;//旋转
                if((manager->NowboxX==nowPlayWay->boxX)&&(manager->NowboxY==nowPlayWay->boxY)&&(pooltest(manager,nowPlayWay))){nowPlayWay->moveWay[v+1]=0;return 1;}//验证是否达到原位置
                --manager->NowboxY;//目标方块上移
                if(((manager->NowboxY>=0))&&(!checkCollision(manager,0))){//碰撞检测,返回1为碰撞,0为没碰撞;
                    if(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]){
                        ++manager->NowboxY;//目标方块返回下移
                        if(routing(manager,nowPlayWay,v))return 1;//递归 
                    }
                    else ++manager->NowboxY;//目标方块返回下移
                }
                else ++manager->NowboxY;//目标方块返回下移
            }
            --v;
        }
        nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]=1;
        return 0;
    }
           

        manager->NowboxX=nowPlayWay->saveMoveX[manager->NowboxY];//读取X坐标位置;类似出栈
        --manager->NowboxX;//目标方块左移
        ++v;if(v>90)return 0;//对方块移动次数进行判定              
        while(!(nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]||checkCollision(manager,0))){//碰撞检测,返回1为碰撞,0为没碰撞     
            nowPlayWay->moveWay[v]=77;//向左   
            if((manager->NowboxX==nowPlayWay->boxX)&&(manager->NowboxY==nowPlayWay->boxY)&&(pooltest(manager,nowPlayWay))){nowPlayWay->moveWay[v+1]=0;return 1;}//验证是否达到原位置
            --manager->NowboxY;//目标方块上移
                if(((manager->NowboxY>=0))&&(!checkCollision(manager,0))&&(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY])){//碰撞检测,返回1为碰撞,0为没碰撞
                ++manager->NowboxY;//目标方块返回下移
                if(routing(manager,nowPlayWay,v))return 1;//递归 
            }
            else ++manager->NowboxY;//目标方块返回下移
            
            for(int j=0;j<nowPlayWay->rotateFrequency;j++){
                rotatePoolTetris(manager,0,0);//旋转,不进行旋转碰撞检测;放止manager->NowboxX的值被改变
                ++v;if(v>90)return 0;//对方块移动次数进行判定
                if(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]){//碰撞检测,返回1为碰撞,0为没碰撞
                    nowPlayWay->moveWay[v]=9;//旋转
                    if((manager->NowboxX==nowPlayWay->boxX)&&(manager->NowboxY==nowPlayWay->boxY)&&(pooltest(manager,nowPlayWay))){nowPlayWay->moveWay[v+1]=0;return 1;}//验证是否达到原位置
                    --manager->NowboxY;//目标方块上移
                    if(((manager->NowboxY>=0))&&(!checkCollision(manager,0))){//碰撞检测,返回1为碰撞,0为没碰撞;
                        if(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]){
                            ++manager->NowboxY;//目标方块返回下移
                            if(routing(manager,nowPlayWay,v))return 1;//递归 
                        }
                        else ++manager->NowboxY;//目标方块返回下移
                    }
                    else ++manager->NowboxY;//目标方块返回下移
                }
                --v;
            }
            nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]=1;            
            --manager->NowboxX;//目标方块左移
            ++v;if(v>90)return 0;//对方块移动次数进行判定   
        }
        v-=(nowPlayWay->saveMoveX[manager->NowboxY]-manager->NowboxX);
        
        manager->NowboxX=nowPlayWay->saveMoveX[manager->NowboxY];//读取X坐标位置;类似出栈
        ++manager->NowboxX;//目标方块右移
        ++v;if(v>90)return 0;//对方块移动次数进行判定              
        while(!(nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]||checkCollision(manager,0))){//碰撞检测,返回1为碰撞,0为没碰撞     
            nowPlayWay->moveWay[v]=75;//向右   
            if((manager->NowboxX==nowPlayWay->boxX)&&(manager->NowboxY==nowPlayWay->boxY)&&(pooltest(manager,nowPlayWay))){nowPlayWay->moveWay[v+1]=0;return 1;}//验证是否达到原位置  
            --manager->NowboxY;//目标方块上移
               if(((manager->NowboxY>=0))&&(!checkCollision(manager,0))&&(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY])){//碰撞检测,返回1为碰撞,0为没碰撞
               ++manager->NowboxY;//目标方块返回下移
                if(routing(manager,nowPlayWay,v))return 1;//递归 
            }
            else ++manager->NowboxY;//目标方块返回下移
            
            for(int k=0;k<nowPlayWay->rotateFrequency;k++){
                rotatePoolTetris(manager,0,0);//旋转,不进行旋转碰撞检测;放止manager->NowboxX的值被改变
                ++v;if(v>90)return 0;//对方块移动次数进行判定
                if(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]){//碰撞检测,返回1为碰撞,0为没碰撞
                    nowPlayWay->moveWay[v]=9;//旋转
                    if((manager->NowboxX==nowPlayWay->boxX)&&(manager->NowboxY==nowPlayWay->boxY)&&(pooltest(manager,nowPlayWay))){nowPlayWay->moveWay[v+1]=0;return 1;}//验证是否达到原位置
                    --manager->NowboxY;//目标方块上移
                    if(((manager->NowboxY>=0))&&(!checkCollision(manager,0))){//碰撞检测,返回1为碰撞,0为没碰撞;
                        if(!nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]){
                            ++manager->NowboxY;//目标方块返回下移
                            if(routing(manager,nowPlayWay,v))return 1;//递归 
                        }
                        else ++manager->NowboxY;//目标方块返回下移
                    }
                    else ++manager->NowboxY;//目标方块返回下移
                }
                --v;
            }
            nowPlayWay->saveMove[manager->NowboxX][manager->NowboxY]=1;
            ++manager->NowboxX;//目标方块右移
            ++v;if(v>90)return 0;//对方块移动次数进行判定   
            
        }
    v -=(manager->NowboxX-nowPlayWay->saveMoveX[manager->NowboxY]); 
    ++manager->NowboxY;//目标方块返回下移
    return 0;
}



void existencePlace(gameManager *manager,autoPlayWay *bestPlayWay){//AI;方块的存在位置
    autoPlayWay nowPlayWay;
    static gameManager backM;
    int deltaX;//方块目标水平位置与原水平位置之差
    bestPlayWay->value=0;//初始化最佳估值
    bestPlayWay->priority=-10000;//初始化最佳优先级
	nowPlayWay.boxX=manager->NowboxX;nowPlayWay.boxY=manager->NowboxY;//记录原坐标
	memcpy(nowPlayWay.pool,manager->pool,16*sizeof(bool));//存储原游戏池
	
	switch(manager->NowboxShape){
		case 0: case 5: case 6:nowPlayWay.rotateFrequency=2;break;//I型方块,Z型方块,S型方块,有两种旋转状态
		case 1: case 3: case 4:nowPlayWay.rotateFrequency=4;break;//T型方块,L型方块,J型方块,有两种旋转状态
		case 2: nowPlayWay.rotateFrequency=1;break;//O型方块,有一种旋转状态
		default: break;
    }
    for(;manager->NowboxY<poolHidth-1;manager->NowboxY++){
        for(manager->NowboxX=1;manager->NowboxX<poolWidth-1;manager->NowboxX++){          
            for(int k=0;k<nowPlayWay.rotateFrequency;k++){
                rotatePoolTetris(manager,0,1);//旋转,不进行旋转碰撞检测;放止manager->NowboxX的值被改变
                if(!checkCollision(manager,0)){//碰撞检测;返回1为碰撞,0为没碰撞
                    manager->NowboxY++;//将列坐标下移,检测是否落地,有碰撞为落地,无碰撞为没落地
                    if(checkCollision(manager,0)){
                        manager->NowboxY--;//返回原列坐标
                        memcpy(&backM,manager,sizeof(gameManager));//再次对游戏数据备份
                        memset(nowPlayWay.saveMove,0,sizeof(nowPlayWay.saveMove));//初始化移动路径 
                        memset(nowPlayWay.moveWay,0,sizeof(nowPlayWay.moveWay));//初始化AI操作路径  
                        nowPlayWay.saveMoveX[backM.NowboxY]=backM.NowboxX;
                        if(routing(&backM,&nowPlayWay,0)){//backM里存储目标地址,nowPlayWay存储原地址
							memcpy(&backM,manager,sizeof(gameManager));//再次对游戏数据备份
                            nowPlayWay.value=100000+evaluate(&backM);//进行估值
                          //  printf("%d ",nowPlayWay.value);
                            
                            if(bestPlayWay->value<nowPlayWay.value){//比较权值
                                memcpy(bestPlayWay,&nowPlayWay,sizeof(autoPlayWay));//将权值较高的复制到bestPlayWay
                            }
                            else if(bestPlayWay->value==nowPlayWay.value){//如果权值相同,比较优先级
                                deltaX=manager->NowboxX-nowPlayWay.boxX;//方块目标水平位置与原水平位置之差
                                if(deltaX>0)//落于右侧的摆法
                                    nowPlayWay.priority=100*deltaX+k;//100 * 水平平移格子数 + 旋转次数;
                                else //落于左侧的摆法
                                    nowPlayWay.priority=100*(-deltaX)+10+k;//100 * 水平平移格子数 + 10 + 旋转次数;
                                if(nowPlayWay.priority>bestPlayWay->priority)//优先级比较
                                    memcpy(bestPlayWay,&nowPlayWay,sizeof(autoPlayWay));//将权值较高的复制到bestPlayWay                         
                            }
						}
                    }
                    else manager->NowboxY--;//返回原列坐标
                }    
            }
        }
    }
}
void changeMoveWay(const gameManager *manager,autoPlayWay *bestPlayWay){//处理改变移动路径
	static gameManager backManager;
	memcpy(&backManager,manager,sizeof(gameManager));//虚拟游戏数据
	existencePlace(&backManager,bestPlayWay);//AI;方块的存在位置
	
	int i=0,j=0;
//	int m;
	int backMoveWay[100];

	
	i=1;
	j=2;
	if(bestPlayWay->moveWay[1]==80){//对移动路径进行落地优化
		bestPlayWay->moveWay[1]=32;
		
		while(bestPlayWay->moveWay[++i]==80);
		while(bestPlayWay->moveWay[i]){
			bestPlayWay->moveWay[j]=bestPlayWay->moveWay[i];
			++i;++j;
		}
	bestPlayWay->moveWay[j]=0;
	}

	memcpy(&backMoveWay,bestPlayWay->moveWay,100*sizeof(int));//复制移动路径数据
	i=0;j=0;
	while(backMoveWay[++i]);//到达移动路径末端

    while(backMoveWay[--i]){//对移动路径进行修改,还原
        if(backMoveWay[i]==9)//解析旋转
            bestPlayWay->moveWay[j]=72;
        else 
            bestPlayWay->moveWay[j]=backMoveWay[i];
		j++;
    }
	bestPlayWay->moveWay[j]=0;
}
void autoPlay(gameManager *manager,gameMainData *mainData){//挂载AI
	autoPlayWay PlayWay;

	while(manager->gameDead){
		changeMoveWay(manager,&PlayWay);
		int i=0;
		while(PlayWay.moveWay[i]){
			keyControl(manager,PlayWay.moveWay[i],mainData); // 按键
			Sleep(200);
			i++;
		}
	
	}
 /*   for(int m=0;m<100;m++)
		printf("%d_",PlayWay.moveWay[m]);
	getch();   */
    

    
}


void easyExistencePlace(gameManager *manager,autoPlayWay *bestPlayWay){//简单AI寻址
    gameManager backManager;
    autoPlayWay nowPlayWay;
    int deltaX;//方块目标水平位置与原水平位置之差
    int k;
    bestPlayWay->value=0;//初始化最佳估值
    bestPlayWay->priority=-10000;//初始化最佳优先级
	nowPlayWay.boxX=manager->NowboxX;nowPlayWay.boxY=manager->NowboxY;//记录原坐标
    memcpy(nowPlayWay.pool,manager->pool,16*sizeof(bool));//存储原游戏池
    
	switch(manager->NowboxShape){
		case 0: case 5: case 6:nowPlayWay.rotateFrequency=2;break;//I型方块,Z型方块,S型方块,有两种旋转状态
		case 1: case 3: case 4:nowPlayWay.rotateFrequency=4;break;//T型方块,L型方块,J型方块,有两种旋转状态
		case 2: nowPlayWay.rotateFrequency=1;break;//O型方块,有一种旋转状态
		default: break;
    }
    for(int i=0;i<nowPlayWay.rotateFrequency;i++){// 尝试各种旋转状态      
        for(int j=1;j<poolWidth-1;j++){// 尝试每一列
            memcpy(&backManager,manager,sizeof(gameManager));//虚拟游戏数据
            backManager.NowboxX=j;//列坐标
            for(k=0;k<i;k++){
                nowPlayWay.moveWay[k]=72;
                rotatePoolTetris(manager,0,1);//旋转,进行旋转碰撞检测;放止manager->NowboxX的值被改变
            }
            deltaX=j-nowPlayWay.boxX;//方块目标水平位置与原水平位置之差
            manager->NowboxX=j;
            if(deltaX>0){
                for(int m=0;m<deltaX;m++){
                    
                }
            }
        
        }
    
    
    }
    
    
    
    
    

}
void easyAI(gameManager *manager,gameMainData *mainData){//临时简单AI
    autoPlayWay PlayWay;
	while(manager->gameDead){
		easyExistencePlace(manager,&PlayWay);
		int i=0;
		while(PlayWay.moveWay[i]){
			keyControl(manager,PlayWay.moveWay[i],mainData); // 按键
			Sleep(200);
			i++;
		}
	
	}

}


HWND GetConsoleHwnd(void){// 获取控制台窗口句柄
	#define MY_BUFSIZE 1024 // Buffer size for console window titles.
	HWND hwndFound; // This is what is returned to the caller.
	char pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated
	char pszOldWindowTitle[MY_BUFSIZE]; // Contains original
	GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);//获取控制台标题存入pszOldWindowTitle中
	//设置控制台的标题为 时间数 进程ID
	wsprintf(pszNewWindowTitle,"%d/%d",//把获取的数目，和当前进程ID输出到缓冲区pszNewwindowtitle中
	GetTickCount(),//用于获取操作系统启动后的毫秒数
	GetCurrentProcessId());//获取当前进程ID
	SetConsoleTitle(pszNewWindowTitle);//更改控制台的标题为新内容
	Sleep(10);
	hwndFound=FindWindow(NULL, pszNewWindowTitle);//获取控制台的HWND号
	SetConsoleTitle(pszOldWindowTitle);//更改控制台的标题为原始内容
	return(hwndFound);//返回句柄
}
void drawBorder(){//GDI绘图绘边框
	HWND hWnd=GetConsoleHwnd();//获取控制台窗口句柄，用于设置画笔颜色
	HDC hDC=GetDC(hWnd);
	HPEN hNewPen=CreatePen(PS_SOLID,1,RGB(0,0,255));//创建画笔对象
	HPEN hOldPen=(HPEN)SelectObject(hDC,hNewPen);//选取画笔对象进行绘图
	CONSOLE_SCREEN_BUFFER_INFO bInfo; // 窗口信息
	HANDLE Hout=GetStdHandle(STD_OUTPUT_HANDLE);//获取控制台句柄
	GetConsoleScreenBufferInfo(Hout, &bInfo );//获取控制台信息
	SetBkMode(hDC, TRANSPARENT);SetTextColor(hDC,255*255);
	for(int i=0;i<6;i++){//横线
		MoveToEx(hDC,200,275+i*32,NULL);//起点
		LineTo(hDC,480,275+i*32);//终点
	}
	MoveToEx(hDC,195,270,NULL);LineTo(hDC,485,270);
	MoveToEx(hDC,195,440,NULL);LineTo(hDC,485,440);
	MoveToEx(hDC,190,265,NULL);LineTo(hDC,490,265);
	MoveToEx(hDC,190,445,NULL);LineTo(hDC,490,445);
	MoveToEx(hDC,200,275,NULL);LineTo(hDC,200,435);//竖线
	MoveToEx(hDC,480,275,NULL);LineTo(hDC,480,435);
	MoveToEx(hDC,195,270,NULL);LineTo(hDC,195,440);
	MoveToEx(hDC,485,270,NULL);LineTo(hDC,485,440);
	MoveToEx(hDC,190,265,NULL);LineTo(hDC,190,445);
	MoveToEx(hDC,490,265,NULL);LineTo(hDC,490,445);
	ReleaseDC(hWnd,hDC);
}
void drawELSFK(){//画出俄罗斯方块的星阵
	char elsfk[]="082009A80F24112011243FFE5120912011A413181D1011301148118A1506120200047FFE4444444444447FFC4204020007F00810142062C0010006001800E0002208221C7F60224022403E40227E22483E4822482248FF4800881508220840080400030001000004FFFE0400041007F804100410041008100810101020A04040108010801080108813FCFC8810881088108817FE1C80F14041200210040E0804";//存储俄罗斯方块五个汉字的字库区位码，使用ucdos的字库转换
	for(int i=0;i<5;i++)//五个汉字
		for(int j=0;j<16;j++){
			for(int k=0;k<4;k++)
				switch(elsfk[i*64+j*4+k]){
					case '0': printf("    ");break;
					case '1': printf("   *");break;
					case '2': printf("  * ");break;
					case '3': printf("  **");break;
					case '4': printf(" *  ");break;
					case '5': printf(" * *");break;
					case '6': printf(" ** ");break;
					case '7': printf(" ***");break;
					case '8': printf("*   ");break;
					case '9': printf("*  *");break;
					case 'A': printf("* * ");break;
					case 'B': printf("* **");break;
					case 'C': printf("**  ");break;
					case 'D': printf("** *");break;
					case 'E': printf("*** ");break;
					case 'F': printf("****");break;
					default:break;
			};
			gotoxy(i*16,j,1);
		}
}
int mainGameMenu(){//游戏主菜单
	int ch;
	int flag=1;
	static const char *menuWord[] = { "1.新游戏", "2.游戏帮助","3.排行榜","4.关于","5.退出" };
	system("cls");
	HWND hWnd=GetConsoleHwnd();//获取控制台窗口句柄
	CONSOLE_CURSOR_INFO cursorInfo = { 1, FALSE };  // 光标信息
    SetConsoleCursorInfo(hConsole, &cursorInfo);  // 设置光标隐藏
	drawELSFK();//画出俄罗斯方块的星阵
	drawBorder();
	SetConsoleTextAttribute(hConsole,0x0f);gotoxy(35,18,1);printf("%s",  menuWord[0] );
	SetConsoleTextAttribute(hConsole,0xf0);
	for(int i=1;i<5;i++){
		gotoxy(35,18+i*2,1);
		printf("%s",  menuWord[i] );
	}
	do{
		ch=_getch();
        switch(ch){
			case 72:flag--;if(flag==0)flag=5;break;// 上 
			case 80:flag++;if(flag==6)flag=1;break;// 下
			case 13:InvalidateRect(hWnd,NULL,FALSE);//清除GDI画图画出全部线条
					system("cls");
					return flag;
			default:break;
		}
		if(flag==1){
				SetConsoleTextAttribute(hConsole,0x0f);gotoxy(35,18,1);printf("%s",  menuWord[0] );
				SetConsoleTextAttribute(hConsole,0xf0);gotoxy(35,26,1);printf("%s",  menuWord[4] );
													   gotoxy(35,20,1);printf("%s",  menuWord[1] );
		}
		else if(flag==2){
				SetConsoleTextAttribute(hConsole,0x0f);gotoxy(35,20,1);printf("%s",  menuWord[1] );
				SetConsoleTextAttribute(hConsole,0xf0);gotoxy(35,18,1);printf("%s",  menuWord[0] );
													   gotoxy(35,22,1);printf("%s",  menuWord[2] );
            }
 		else if(flag==3){
				SetConsoleTextAttribute(hConsole,0x0f);gotoxy(35,22,1);printf("%s",  menuWord[2] );
				SetConsoleTextAttribute(hConsole,0xf0);gotoxy(35,20,1);printf("%s",  menuWord[1] );
													   gotoxy(35,24,1);printf("%s",  menuWord[3] );
            }
 		else if(flag==4){
				SetConsoleTextAttribute(hConsole,0x0f);gotoxy(35,24,1);printf("%s",  menuWord[3] );
				SetConsoleTextAttribute(hConsole,0xf0);gotoxy(35,22,1);printf("%s",  menuWord[2] );
													   gotoxy(35,26,1);printf("%s",  menuWord[4] );
            }
 		else if(flag==5){
				SetConsoleTextAttribute(hConsole,0x0f);gotoxy(35,26,1);printf("%s",  menuWord[4] );
				SetConsoleTextAttribute(hConsole,0xf0);gotoxy(35,24,1);printf("%s",  menuWord[3] );
													   gotoxy(35,18,1);printf("%s",  menuWord[0] );
            }
    } while (1);
}
void mainGameMenuChoice(gameManager *manager,gameMainData *mainData){//游戏菜单选择
	startGameInfo(manager,mainData);//初始化游戏框架
	while(1)
		switch(mainGameMenu()){
			case 1:runTetrisGame(manager,mainData);break;
			case 2:gameHelp();break;
			case 3:printfGameData(mainData);break;
			case 4:MessageBox(NULL,TEXT("20150512 BY:冬雪春梦\n\t版本信息：\n\t\t俄罗斯方块1.02"), TEXT("关于"), MB_OK);break;
			case 5:return;
			default:break;
		}
}
void gameHelp(){//游戏帮助说明
	gotoxy(1,1,1);
	printf("\n\n\n");
	printf("\t\t┏━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("\t\t┃    ============H-E-L-P=============          ┃\n");
	printf("\t\t┃                旋转方块                      ┃\n");
	printf("\t\t┃                   ↑	                        ┃\n");
	printf("\t\t┃     向左移动方块←  →向右移动方块           ┃\n");//这里可能由于这"←" "→"两个符号，不同编译器的占格不同。
	printf("\t\t┃                   ↓	                        ┃\n");
	printf("\t\t┃              向下移动方块                    ┃\n");
	printf("\t\t┃   '  '空格直接落地      ESC暂停游戏          ┃\n");
	printf("\t\t┃ 游戏中按'Z'或'z'键，游戏直接结束，返回主菜单 ┃\n");
	printf("\t\t┃                                              ┃\n");
	printf("\t\t┃                             按任意键返回     ┃\n");
	printf("\t\t┗━━━━━━━━━━━━━━━━━━━━━━━┛\n");
	getch();
}
int main(){
	gameManager gameM;
	gameMainData gameD;
	hConsole=GetStdHandle(STD_OUTPUT_HANDLE);//获取控制台输出句柄
	system("mode con cols=80 lines=30");               //设置控制台宽度,长度
	system("color f0");               //设置控制台颜色
	mainGameMenuChoice(&gameM,&gameD);//游戏选择主菜单
return 0;
}
