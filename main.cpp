#include <cstring>
#include "time.h"
#include "buffer.h"
#include "const.h"
#include <assert.h>
#include "argparse.h"
void str2int(int &int_temp, const string &string_temp)
{
	stringstream stream(string_temp);
	stream >> int_temp;
}

vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if ("" == str) return res;
	char * strs = new char[str.length() + 1];
	strcpy(strs, str.c_str());

	char * d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());

	char *p = strtok(strs, d);
	while (p) {
		string s = p; 
		res.push_back(s);
		p = strtok(NULL, d);
	}
	delete strs, d;
	return res;
}


void CreateBlockWRTest(int num_blocks)
{
	FILE * fstream_w;
	char dbfile[] = "data.dbf";
	fstream_w = fopen(dbfile, "wb");
	if (fstream_w == NULL)
	{
		printf("open or create file failed!\n");
		exit(1);
	}
	else {
		cout << "Open " << dbfile << " successfully!" << endl;
	}
	unsigned int block_num = 0;
	unsigned int btimestamp = 0;
	unsigned short int offset[FRAMESIZE / RECORDSIZE];
	int BLOCK_HEAD = sizeof(btimestamp) + sizeof(offset);
	for (int i = 0; i < FRAMESIZE / RECORDSIZE; i++) {
        offset[i] = BLOCK_HEAD + i * RECORDSIZE;
    };

	for (int j = 0; j < num_blocks; j++) {
		block_num = j;
		int writeCnt = fwrite(&block_num, sizeof(block_num), 1, fstream_w); // 数据块号
		writeCnt = fwrite(&btimestamp, sizeof(btimestamp), 1, fstream_w); // 时间戳
		writeCnt = fwrite(&offset, sizeof(offset), 1, fstream_w); // 块内偏移表
		char * p_schema = NULL;
		unsigned int timestamp = j;
		unsigned int length = RECORDSIZE;
		char namePtr[32];
		char adrressPtr[256];
		char genderPtr[4];
		char birthdayPtr[12];
        size_t page_header_size = sizeof(block_num) + sizeof(btimestamp) + sizeof(offset);
		for (int i = 0; i < FRAMESIZE / RECORDSIZE; i++) {
			writeCnt = fwrite(&fstream_w, sizeof(fstream_w), 1, fstream_w);
			writeCnt = fwrite(&length, sizeof(length), 1, fstream_w);
			writeCnt = fwrite(&timestamp, sizeof(timestamp), 1, fstream_w);
			writeCnt = fwrite(&namePtr, sizeof(namePtr), 1, fstream_w);
			writeCnt = fwrite(&adrressPtr, sizeof(adrressPtr), 1, fstream_w);
			writeCnt = fwrite(&genderPtr, sizeof(genderPtr), 1, fstream_w);
			writeCnt = fwrite(&birthdayPtr, sizeof(birthdayPtr), 1, fstream_w);
		}
        size_t actual_recordsize = sizeof(fstream_w) + sizeof(length) + sizeof(timestamp)\
                                   + sizeof(namePtr) + sizeof(adrressPtr) + sizeof(genderPtr)\
                                   + sizeof(birthdayPtr); 
		char null[224];
		writeCnt = fwrite(&null, sizeof(null), 1, fstream_w);
	}

	fclose(fstream_w);
}

void processTrace(int page_id, int operation, DBSAccesser* accesser) {
	if (operation == 0)
	{
#if VERBOSE
		cout << "---[trace read] Page_id:" << page_id << " ---" << endl;
#endif
		accesser->read(page_id - 1);
	}
	else if(operation == 1)
	{
#if VERBOSE
		cout << "---[trace write] page_id:" << page_id << " ---" << endl;
#endif
        char null[FRAMESIZE];
		accesser->write(page_id - 1, null);
	}
}


int main(int argc, char const* argv[]){
	auto args = util::argparser("Storage and buffer manager.");
    args.set_program_name("test")
        .add_help_option()
        .add_option<string>("-p", "--policy", "Buffer replacement policy. Candidates [LRU, LRU2, CLOCK]", "LRU")
        .parse(argc, argv);

	CreateBlockWRTest(50000);
	string tracefilename = "data-5w-50w-zipf.txt";
	ifstream traceFile(tracefilename.c_str());
	if (!traceFile)
	{
		cout << "Error opening " << tracefilename << " for input" << endl;
		exit(-1);
	}
	else {
		cout << "Open " << tracefilename << " successfully!" << endl;
	}

    const string policy = "--policy";
	DBSAccesser accesser = DBSAccesser("data.dbf", args.get_option<string>(policy));

	int wr = 0;
	int page_id = 0;
	string trace;
	int request_num = 0;
	cout << "BUFFER IO PERFORMANCE TESTING!\n";
    clock_t start, t_end; double duration;
	start = clock();
	while (!traceFile.eof()) // !file.eof()
	{
		getline(traceFile, trace);
		std::vector<string> res = split(trace, ",");
		str2int(wr, res[0]);
		str2int(page_id, res[1]);
		processTrace(page_id, wr, &accesser); 

		request_num++;
	}
	t_end = clock();
    accesser.printStat();
    duration = (t_end - start)*1e-6;
	printf("trace consuming time: %f seconds\n", duration);
	traceFile.close();
    return 1;
}


