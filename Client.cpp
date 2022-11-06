#include "Date.h"
#include "Sstring.h"
#include "FileInfo.h"
#include "LocalUser.h"
#include "DirIterator.h"
#include "IceException.h"
#include "LocalTuneItem.h"

#include <stack>
#include <locale>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <tchar.h>
#include <conio.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>

#include "ThreadPool.h"

#include "libzplay.h"

#pragma comment(lib, "lib\\libzplay.lib")

using namespace std;

enum FieldEnum {
	Username,
	Listname,
	Musicname
};


// 全局状态下的命令
const vector<sweet::Sstring> globalCmds = {
	L"reg",
	L"login",
	L"exit"
}
// 用户状态下的命令
, userCmds = {
	L"info",
	L"mdl",
	L"create",
	L"delete",
	L"into"
}
// 歌单状态下的命令
, playListCmds = {
	L"add",
	L"remove",
	L"play",
	L"mdt",
	L"back"
}
, playingCmds = {
	L"pause",
	L"continue"
}
, supportExtensions = {
	L"mp3",
	L"mav",
	L"flac"
};
vector<sweet::Sstring>		allCmds;

icemusic::LocalUserManager *userManager = nullptr;
vector<sweet::Sstring>		errorList;
vector<sweet::Sstring>		logList;
unique_ptr<icemusic::LocalUser>
							currentUser(nullptr);
extern vector<Sstring> icemusic::databaseErrLog;
// 字符串特征搜索表
map<Sstring, FieldEnum>		fieldMap;
Sstring						curList;		// 当前歌单
icemusic::LocalTuneItem		*tunePlaying;	// 正在播放的音乐
unique_ptr<libZPlay::ZPlay>	player(nullptr);			// 全局播放器

enum class enumCmds {
	Unknown,

	Reg, Login, Exit,

	Info, MdL, Create, Delete, Into,

	Add, Remove, Play, MdT, Back,

	Pause, Continue
};

using sweet::SstringArray;
using sweet::Sstring;

struct CommandInfo {
	enumCmds		id;
	SstringArray	arguments;
};

// parse回调类型
// 每个命令的parser都被定义为void(SstringArray&)的函数
typedef function<void(SstringArray&)> ArgFunc;
std::vector<ArgFunc> parserList;

void pushErr(const string &msg);				// 推入错误消息
void pushErr(const Sstring &msg);				
void showMsg(const string &msg);
void showMsg(const Sstring &msg);				// 在黑框框上显示消息
void showPrompt();								// 显示提示符
void showErr();									// 显示错误消息
void showUserInfo();							// 显示当前用户属性
void showListInfo(const Sstring &listname);		// 显示歌单属性
void showTuneInfo(
	icemusic::LocalTuneItem *ptune				// 显示音乐属性
);
icemusic::LocalTuneItem *getTune(				// 根据str得到tune
	const Sstring &str
);
void initCmds();								// 初始化命令
void initParsers();								// 初始化parser
void initUser(icemusic::LocalUser *uptr);		// 初始化用户
void initPlayer();								// 初始化player.
void init();									// 初始化
void loadFieldMap(icemusic::LocalUser *uptr);	// 加载用户的字符串特征表
bool checkLogin();								// 检查当前用户
void pushUselessArg(							// 处理无效参数
	SstringArray &args,
	icemusic::LocalUser::sizeType start
);
void pushDatabaseLog();							// 将database的错误日志压入errlist
Sstring getInput();								// 从终端获取输入
SstringArray split(
	const Sstring &str,
	const char ch
);												// 根据字符分割
CommandInfo getInfo(const Sstring &cmd);		// 根据字符串取得CmdInfo
void parse(CommandInfo &info);					// 根据info的内容调用不同的parser
void playWithPath(const Sstring &path);			// 根据path播放音乐
void pushLog(const Sstring &msg);				// 稽日志
void saveLog();									// 保存日志

// 各种命令的解析函数
void parseUnknown(SstringArray &arguments);			// 解析unknown指令
void parseReg(SstringArray &arguments);				// 解析reg指令
void parseLogin(SstringArray &arguments);			// 解析login指令
void parseExit(SstringArray &arguments);			// 解析exit指令
void parseInfo(SstringArray &arguments);			// 解析info指令
void parseMdl(SstringArray &aruments);				// 解析mdl命令
void parseCreate(SstringArray &arguments);			// 解析create命令
void parseDelete(SstringArray &arguments);			// 解析delete命令
void parseInto(SstringArray &arguments);			// 解析into命令
void parseAdd(SstringArray &arguments);				// 解析add命令
void parseRemove(SstringArray &arguments);			// 解析remove命令
void parsePlay(SstringArray &arguments);			// 解析play命令
void parseMdt(SstringArray &arguments);				// 解析mdt命令
void parseBack(SstringArray &arguments);			// 解析back命令
void parsePause(SstringArray &arguments);			// 解析pause命令
void parseContinue(SstringArray &arguments);		// 解析continue命令

int _tmain(int argc, TCHAR *argv[]) {
	atexit(saveLog);
	set_terminate(saveLog);
	init();
	do {
		showPrompt();
		parse(getInfo(getInput()));
		showErr();
	} while (true);
}

void pushErr(const string &msg) {
	pushErr(Sstring(msg.begin(), msg.end()));
}

void pushErr(const Sstring &msg) {
	errorList.push_back(msg);
}

void showMsg(const string &msg) {
	cout << msg;
}

void showMsg(const Sstring &msg) {
	wcout << msg;
}

void showPrompt() {
	if (currentUser == nullptr) {
		cout << "> ";
	}
	else {
		wcout << currentUser->name() << "> ";
	}
}

void showErr() {
	if (errorList.empty())
		cout << endl;
	for (auto error : errorList)
		wcout << error << endl;
	errorList.clear();
}

void showUserInfo() {
	cout << endl;
	setiosflags(ios::left);
	wcout
		<< setw(20) << "username: " << currentUser->name() << endl
		<< setw(20) << "n(lists): " << currentUser->getListCount() << endl << endl;


	setiosflags(ios::right);
	auto count = currentUser->getListCount();
	for (auto i = count & 0; i < count; ++i) {
		wcout << i << ": " << setw(20) << currentUser->getList(i).name() << endl;
	}
}

void showListInfo(const Sstring &listname) {
	cout << endl;
	setiosflags(ios::left);

	wcout
		<< setw(20) << "listname: " << listname << endl
		<< setw(20) << "index:" << currentUser->getListIndex(listname) << endl
		<< setw(20) << "count:" << currentUser->getList(listname).size() << endl;

	wcout << endl;
	
	wcout << setw(20) << "title" << setw(40) << "directory" << endl << endl;
	setiosflags(ios::left);

	auto listIns = currentUser->getList(listname);
	int i = 0;

	for (auto itr = listIns.begin(); itr != listIns.end(); ++itr) {
		wcout << i++ << ":"
			<< setw(20) << (*itr)->getProperty().title << setw(40) << (*itr)->directory() << endl;
	}
}

void showTuneInfo(icemusic::LocalTuneItem *ptune) {
	
}


void initCmds() {
	allCmds.push_back(L"unknown");	// 第一个占位
	for (auto x : globalCmds)
		allCmds.push_back(x);
	pushLog(L"装载动力核心装置...  	20%!");

	for (auto x : userCmds)
		allCmds.push_back(x);
	pushLog(L"装载动力核心装置...		40%!");

	for (auto x : playListCmds)
		allCmds.push_back(x);
	pushLog(L"装载动力核心装置...		60%!");

	for (auto x : playingCmds)
		allCmds.push_back(x);
	pushLog(L"装载动力核心装置...		COMPLETE！");
}

void initParser() {
	pushLog(L"装载后台解释器.......  全力加载中");
	parserList.push_back(parseUnknown);
	parserList.push_back(parseReg);
	parserList.push_back(parseLogin);
	parserList.push_back(parseExit);
	parserList.push_back(parseInfo);
	parserList.push_back(parseMdl);
	parserList.push_back(parseCreate);
	parserList.push_back(parseDelete);
	parserList.push_back(parseInto);
	parserList.push_back(parseAdd);
	parserList.push_back(parseRemove);
	parserList.push_back(parsePlay);
	parserList.push_back(parseMdt);
	parserList.push_back(parseBack);
	parserList.push_back(parsePause);
	parserList.push_back(parseContinue);
	pushLog(L"解释器装载完毕！");
}

void initUser(icemusic::LocalUser *uptr) {
	pushLog(L"初始化暗黑♂的映射特征表...");
	loadFieldMap(uptr);
	pushLog(L"映射特征表初始化完毕！");
}

void initPlayer() {
	pushLog(L"初始化播放器...");
	using namespace libZPlay;
	player.reset(CreateZPlay());
	pushLog(L"播放器初始化完毕");
}

void init() {
	pushLog(L"(#`O′)初始化某辣鸡到掉渣的音乐播放器中...");
	std::locale::global(std::locale(""));
	initCmds();
	// 取得用户管理器的实例
	userManager = icemusic::LocalUserManager::getInstance();
	if (userManager == nullptr) {
		pushErr("Failed to get the instance of user manager.");
		cin.get();
		exit(EXIT_FAILURE);
	}
	initParser();
	initPlayer();
	pushLog(L"初始化音乐播放器完毕! 随时待命！");
}

void loadFieldMap(icemusic::LocalUser *uptr) {
	fieldMap.clear();	// 清空原有记录

	// 记载用户名
	fieldMap.insert(make_pair(uptr->name(), Username));
	
	auto count	 = uptr->getListCount();
	for (auto i = count & 0; i < count; ++i) {
		auto listIns = uptr->getList(i);
		// 记载歌单名
		fieldMap.insert(make_pair(listIns.name(), Listname));

		for (auto each : listIns) {
			// 记载歌名
			fieldMap.insert(make_pair(each->path(), Musicname));
			fieldMap.insert(make_pair(each->getProperty().title, Musicname));
		}
	}

}

bool checkLogin() {
	if (currentUser.get() == nullptr) {
		pushErr("no user have logged in.");
		return false;
	}
	return true;
}


void pushUselessArg(
	SstringArray &args, 
	icemusic::LocalUser::sizeType start
) {
	if (args.size() >= start)
		return;

	for (auto i = start; i < args.size(); ++i)
		pushErr(L"useless argument : " + args[i]);
}

void pushDatabaseLog() {
	for (auto x : icemusic::databaseErrLog) {
		pushErr(x);
	}
	icemusic::databaseErrLog.clear();
}

Sstring getInput() {
	Sstring res;
	getline(wcin, res);
	return res;
}

SstringArray split(
	const Sstring &str,
	const char ch
) {
	Sstring tmp;
	SstringArray res;

	for (auto i = str.size() & 0; i < str.size();) {
		if (str[i] == '\'') {
			if (!tmp.empty()) {
				res.push_back(tmp);
				tmp.clear();
			}
			++i;
			while (i < str.size() && str[i] != '\'')
				tmp += str[i++];

			if (i < str.size()) ++i;

			res.push_back(tmp);
			tmp.clear();
			continue;
		}
		if (str[i] == '\"') {
			if (!tmp.empty()) {
				res.push_back(tmp);
				tmp.clear();
			}
			++i;
			while (i < str.size() && str[i] != '\"')
				tmp += str[i++];

			if (i < str.size()) ++i;

			res.push_back(tmp);
			tmp.clear();
			continue;
		}
		if (str[i] == ch && !tmp.empty()) {
			res.push_back(tmp);
			tmp.clear();
			++i;
			continue;
		}
		if (str[i] == ch) {
			i++;
			continue;
		}
		tmp += str[i++];
	}

	if (!tmp.empty())
		res.push_back(tmp);
	return res;
}

CommandInfo getInfo(const Sstring &cmd) {
	SstringArray arr(split(cmd, ' '));
	CommandInfo res{ enumCmds::Unknown, {} };
	if (arr.empty())
		return res;

	auto itr = find(allCmds.begin(), allCmds.end(), arr[0]);
	if (itr == allCmds.end()) {
		pushErr("can't catch any keyword");
		return res;
	}
	// 获得命令的id
	res.id = static_cast<enumCmds>(
		static_cast<int>(
			itr - allCmds.begin()
		)
	);
	
	// 江剩余的参数推入arguments中
	res.arguments = SstringArray(arr.begin() + 1, arr.end());
	return res;
}

void parse(CommandInfo &info) {
	pushLog(L"指令get！ 开始解析 ∑∑∑∑∑" + allCmds[static_cast<int>(info.id)]);
	// 调用对应的parser
	parserList[static_cast<int>(info.id)](info.arguments);
}

void playWithPath(const Sstring &path) {
	pushLog(L"即将启动线程播放淫♂乐: " + path);
	// 取得实例
	icemusic::LocalTuneItem *ins =
		icemusic::TuneItemFactory::getInstance()->getTuneItem(path);

	// 成功
	if (ins != nullptr) {
		// 开一条线程播放淫♂乐
		ThreadPool pool(1);
		std::future<bool> f =
			pool.enqueue([ins] {
			using namespace libZPlay;

			player->OpenFileW(ins->directory().c_str(), sfAutodetect);
			if (player->Play() == 0)
				return false;
			return true;
		}
		);
		// 启动线程
		if (f.get()) {
			tunePlaying = ins;
		}
	}
	else {
		pushErr("can not the music instance. you may type a path which is tan90.");
		return;
	}
}

void pushLog(const Sstring &msg) {
	logList.push_back(msg);
}

void saveLog() {
	Date today = Date::today();
	wostringstream	oss;
	wofstream		of;
	oss << today.year() << "-" << today.month() << "-" << today.day() << ".txt";
	of.open(oss.str(), ios::app | ios::out);
	if (!of.is_open())
		return;

	for (auto eachLog : logList) {
		of << eachLog << endl;
	}

	of.close();
}

void parseUnknown(SstringArray &) {
	pushErr(L"警报！ 警报！ 某未知的邪恶命令...");
}

void parseReg(SstringArray &arguments) {
	if (arguments.empty()) {
		pushErr("reg must need one parament");
		return;
	}
	Sstring username(arguments[0]);
	pushLog(L"您的好友【" + username + L"】正在上线的路上...");
	currentUser = userManager->registerUser(username);
	if (currentUser == nullptr) {
		pushDatabaseLog();
		pushLog(L"您的好友【" + username + L"】在上线的途中不幸遇上三日凌空， 同191号文明毁于太阳的热浪之中...");
		return;
	}

	pushLog(L"您的好友【" + username + L"】上线了");
	// 处理无效参数
	pushUselessArg(arguments, 1);
}

void parseLogin(SstringArray &arguments) {
	if (arguments.empty()) {
		pushErr("login must need one parament");
		return;
	}
	Sstring username(arguments[0]);

	pushLog(L"您的好友【" + username + L"】正在上线的路上...");

	currentUser = userManager->getUser(username);
	if (currentUser == nullptr) {
		pushDatabaseLog();
		return;
	}
	else {
		initUser(currentUser.get());	// 初始化用户
	}
	pushLog(L"您的好友【" + username + L"】上线了");
	
	pushUselessArg(arguments, 1);
}

void parseExit(SstringArray &arguments) {
	for (auto itr = arguments.begin();
		itr != arguments.end(); ++itr) {
		pushErr(L"useless argument : " + *itr);
	}
	showErr();
	showMsg("thanks 4 your using. :-)... ");
	cin.get();
	exit(EXIT_SUCCESS);
}

void parseInfo(SstringArray &arguments) {
	if (!checkLogin())
		return;

	if (arguments.empty()) {	// 如果info没有参数
		showUserInfo();
		return;
	}

	Sstring listname(arguments[0]);

	pushLog(L"暗中观察(查询) " + listname);

	if (fieldMap.count(listname) && fieldMap[listname] == Listname) {
		showListInfo(listname);
	}
	else {
		pushErr(L"there's no such a list named " + listname);
	}
}

void parseMdl(SstringArray &arguments) {
	if (!checkLogin())
		return;
	if (arguments.empty()) {
		pushErr("Mdl must take one argument.");
		return;
	}

	Sstring oldUsername(currentUser->name());
	Sstring newUsername(arguments[0]);

	pushLog(L"【" + oldUsername + L"】使用了 【改名卡】试图江名字改为" + L" 【" + newUsername + L"】");

	if (!icemusic::LocalUser::isValidName(newUsername)) {
		pushLog(L"系统使用了【无懈可击】并说道：\"改名也要按照稽本法!\"");

		pushErr(L"the new name " + newUsername + L" is invalid.");
		return;
	}
	if (!currentUser->changeName(newUsername)) {
		pushErr(L"failed in renaming the current user from " 
			+ currentUser->name() 
			+ L" 2 " + newUsername);
		pushDatabaseLog();
		return;
	}

	pushLog(L"【" + oldUsername + L"】的改名卡生效了！");
}

void parseCreate(SstringArray &arguments) {
	if (!checkLogin())
		return;

	if (arguments.empty()) {
		pushErr("create must take one argument at least.");
		return;
	}
	
	for (auto each : arguments) {

		pushLog(L"创建歌单【" + each + L"】....");

		if (!currentUser->createList(each)) {
			pushErr(L"failed in creating list " + each);
			pushDatabaseLog();

			pushLog(L"创建失败！");
		}

		pushLog(L"创建成功!");
	}
	loadFieldMap(currentUser.get());
}

void parseDelete(SstringArray &arguments) {
	if (!checkLogin())
		return;

	if (arguments.empty()) {
		pushErr("delete must take one argument at least.");
		return;
	}

	for (auto each : arguments) {
		pushLog(L"删除歌单【" + each + L"】....");

		if (!currentUser->deleteList(each)) {
			pushErr(L"failed in deleting list " + each);
			pushDatabaseLog();

			pushLog(L"删除失败！");
		}
		pushLog(L"创建成功!");
	}
	loadFieldMap(currentUser.get());
}

void parseInto(SstringArray &arguments) {
	if (!checkLogin())
		return;

	if (arguments.empty()) {
		pushErr("into must take one argument");
		return;
	}

	Sstring listname(arguments[0]);
	if (fieldMap.count(listname) && fieldMap[listname] == Listname) {
		curList = listname;
		pushLog(L"状态改为 " + listname);
	}
	else {
		pushErr(L"no such a list named " + listname);
	}
}

void parseAdd(SstringArray &arguments) {
	if (!checkLogin())
		return;

	if (arguments.empty()) {
		pushErr("add must take one argument.");
		return;
	}

	if (arguments.size() == 1 && curList.empty()) {
		pushErr("too few argument(s) with add.");
		return;
	}

	// 只有一个参数
	if (arguments.size() == 1) {
		arguments.insert(arguments.begin() + 1, curList);
		return parseAdd(arguments);
	}

	// 有至少俩个参数的情况
	Sstring listname(arguments[0]);
	auto	listCount = arguments.size();

	pushLog(L"添加本地音乐...");

	if (fieldMap.count(listname) && fieldMap[listname]) {
		auto listIndex = currentUser->getListIndex(listname);

		for (decltype(listCount) i = 1; i < listCount; ++i) {
			FileInfo info(arguments[i]);
			if (info.isDir()) {
				DirIterator dItr(info.path(), supportExtensions);
				Sstring		path;
				while (dItr.has()) {
					path = dItr.current();

					if (!currentUser->pushMusic(listIndex, path)) {
						pushErr(L"failed in adding music into the list " + listname);
						pushDatabaseLog();
					}
					else {
						pushLog(L"成功添加音乐... 路径：" + path);
					}
					dItr.next();
				}
			}
			else if (info.exists()) {
				if (!currentUser->pushMusic(listIndex, info.absoluteFilePath())) {
					pushErr(L"failed in adding music into the list " + listname);
					pushDatabaseLog();
				}
				else {
					pushLog(L"成功添加音乐... 路径：" + info.absoluteFilePath());
				}
			}
			else {
				pushErr(info.path() + L" is tan90.");
			}
		}
	}
	else {
		pushErr(L"there's no such a list named " + listname);
	}
	

}

void parseRemove(SstringArray &arguments) {
	if (!checkLogin())
		return;

	if (arguments.empty()) {
		pushErr("remoce must take one argument");
		return;
	}

	Sstring first(arguments[0]);
	
	pushLog(L"删除音乐(从歌单中删除)...");

	if (!curList.empty()) {
		wistringstream iss(first);
		icemusic::LocalUser::sizeType index = -1;

		if (iss >> index) {
			if (index >= 0 && currentUser->getList(
				currentUser->getListIndex(curList)).size() > index) {
				if (!currentUser->popMusic(currentUser->getListIndex(curList), index)) {
					pushErr("failed in removing music from the list.");
					pushDatabaseLog();
					return;
				}
				else {
					pushLog(L"成功删除音乐!");
				}
			}
		}
	}
	else if (arguments.size() > 1) {
		Sstring second(arguments[1]);
		auto listIndex = currentUser->getListIndex(first);
		auto tuneIndex = currentUser->getTuneIndex(listIndex, second);
		if (!currentUser->popMusic(listIndex, tuneIndex)) {
			pushErr("failed in removing music from the list.");
			pushDatabaseLog();
			return;
		}
		else {
			pushLog(L"成功删除音乐!");
		}
	}
	else {
		pushErr("invalid argument of remove");
	}
}

void parsePlay(SstringArray &arguments) {
	if (!checkLogin())
		return;

	if (arguments.empty()) {
		pushErr("play must take one argument");
		return;
	}

	Sstring first(arguments[0]);
	// 可以播放的参数：
	// 1. 如果已经存在于表状态之下 可以使用序号的播放模式
	// 2. 任何时刻都可以使用的	路径播放模式
	
	// 存在于表状态之下
	if (!curList.empty()) {
		wistringstream iss(first);
		icemusic::LocalUser::sizeType index = -1;

		if (iss >> index) {
			if (index >= 0 && currentUser->getList(
			currentUser->getListIndex(curList)).size() > index) {
				playWithPath(
					currentUser->getList(
						currentUser->getListIndex(
							curList
						)
					)[index]->directory()
				);
			}
		}
		else {
			playWithPath(first);
		}
	}
	else {
		playWithPath(first);
	}

	pushUselessArg(arguments, 1);
}

void parseMdt(SstringArray &arguments) {
	if (!checkLogin())
		return;
	
	if (arguments.size() < 2) {
		pushErr("mdt must take 2 arguments.");
		return;
	}
	
	Sstring oldListname(arguments[0]);
	Sstring newListname(arguments[1]);
	if (!currentUser->renameList(
			currentUser->getListIndex(
				oldListname
			), 
		newListname)
	) {
		pushErr(L"failed in renaming " + oldListname + L" 2 " + newListname);
		pushDatabaseLog();
	}
	else {
		pushLog(L"【改名卡】生效于【" + oldListname + L"】变成【" + newListname + L"】");
	}

	pushUselessArg(arguments, 1);
}

void parseBack(SstringArray &arguments) {
	curList.clear();
	pushUselessArg(arguments, 0);
	pushLog(L"状态改为用户状态");
}

void parsePause(SstringArray &arguments) {
	player->Pause();
}

void parseContinue(SstringArray &arguments) {
	player->Play();
}