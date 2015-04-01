#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h> /* Required for _spawnv */
#include <windows.h>
#include <io.h>

#define pipe(h) _pipe(h, 1024*16, _O_BINARY|_O_NOINHERIT)
#define getpid() GetCurrentProcessId()
#define dup _dup
#define fileno _fileno
#define dup2 _dup2
#define close _close
#define read _read
#define write _write
#else
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

using namespace std;

//Формируем в глобальных переменных тело запроса и его длинну
static const string strRequestBody = "===this is request body===\n";
static const string strRequestHeader = "Content-Length=" + to_string((long long)strRequestBody.length());

//Формируем переменные окружения которые будут отосланы дочернему процессу
static const char *pszChildProcessEnvVar[4] = {strRequestHeader.c_str(), "VARIABLE2=2", "VARIABLE3=3", 0};

//Формируем переменные командной строки для дочернего процесса. Первая переменная - путь к дочернему процессу.
static const char *pszChildProcessArgs[4] = {"./Main_Child.exe", "first argument", "second argument", 0};
//При желании можно запустить интерпретатор какого-нибудь скрипта. 
//Тогда первый аргумент - путь к интерпретатору, второй - к скрипту
//static const char *pszChildProcessArgs[3] = {"python", "./test.py", 0};

//Это функция, которая породит дочерний процесс и передаст ему переменные из родительского
int spawn_process(const char *const *args, const char * const *pEnv)
{
#ifdef _WIN32
	return _spawnve(P_NOWAIT, args[0], args, pEnv);
#else
    /* Create copy of current process */
    int pid = fork();
    
    /* The parent`s new pid will be 0 */
    if(pid == 0)
    {
		/* We are now in a child progress 
		Execute different process */
		execvpe(args[0], (char* const*)args, (char* const*)pEnv);

		/* This code will never be executed */
		exit(EXIT_SUCCESS);
	}

    /* We are still in the original process */
    return pid;
#endif    
}

int main()
{
	int fdStdInPipe[2], fdStdOutPipe[2];
	
	fdStdInPipe[0] = fdStdInPipe[1] = fdStdOutPipe[0] = fdStdOutPipe[1] = -1;
	if (pipe(fdStdInPipe) != 0 || pipe(fdStdOutPipe) != 0)
	{
		cout << "Cannot create CGI pipe";
		return 0;
	}

	// Duplicate stdin and stdout file descriptors
	int fdOldStdIn = dup(fileno(stdin));
	int fdOldStdOut = dup(fileno(stdout));

	// Duplicate end of pipe to stdout and stdin file descriptors
	if ((dup2(fdStdOutPipe[1], fileno(stdout)) == -1) || (dup2(fdStdInPipe[0], fileno(stdin)) == -1))
		return 0;

	// Close original end of pipe
	close(fdStdInPipe[0]);
	close(fdStdOutPipe[1]);

	//Запускаем дочерний процесс, отдаем ему переменные командной строки и окружения
	const int nChildProcessID = spawn_process(pszChildProcessArgs, pszChildProcessEnvVar);

	// Duplicate copy of original stdin an stdout back into stdout
	dup2(fdOldStdIn, fileno(stdin));
	dup2(fdOldStdOut, fileno(stdout));

	// Close duplicate copy of original stdin and stdout
	close(fdOldStdIn);
	close(fdOldStdOut);

	//Отдаем тело запроса дочернему процессу
	write(fdStdInPipe[1], strRequestBody.c_str(), strRequestBody.length());

	while (1)
	{
		//Читаем ответ от дочернего процесса
		char bufferOut[100000];
		int n = read(fdStdOutPipe[0], bufferOut, 100000);
		if (n > 0)
		{
			//Выводим ответ на экран
			fwrite(bufferOut, 1, n, stdout);
			fflush(stdout);
		}

		//Если дочерний процесс завершился, то завершаем и родительский процесс
#ifdef _WIN32
		DWORD dwExitCode;
		if (!::GetExitCodeProcess((HANDLE)nChildProcessID, &dwExitCode) || dwExitCode != STILL_ACTIVE)
			break;
#else
		int status;
		if (waitpid(nChildProcessID, &status, WNOHANG) > 0)
			break;
#endif
	}
	return 0;
}