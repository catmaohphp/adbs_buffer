#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <stack>

#include "const.h"

using namespace std;

struct bFrame
{
	char field[FRAMESIZE];
};


struct BCB
{
    BCB() {};
    int page_id;
    int frame_id;
    int count = 0;
    bool referenced = true;
    int dirty = 0;
};


class DSMgr {
    public:
        DSMgr() {};
        
        // OpenFile function is called anytime a file needs to be opened for reading or writing.
        // The prototype for this function is OpenFile(String filename) and returns an error code.
        // The function opens the file specified by the filename.
        void openFile(string Filename);


        // ReadPage function is called by the FixPage function in the buffer manager. This
        // prototype is ReadPage(page_id, bytes) and returns what it has read in. This function calls
        // fseek() and fread() to gain data from a file.
        void readPage(int page_id, bFrame& frame);

        // WritePage function is called whenever a page is taken out of the buffer. The
        // prototype is WritePage(frame_id, frm) and returns how many bytes were written. This
        // function calls fseek() and fwrite() to save data into a file.
        int writePage(int page_id, bFrame& frm);

        int getIONum() {
            return io_num;
        }

    private:
        FILE* currFile = NULL;
        
        int io_num = 0;

};


class Bgr{
    public:
        Bgr() {
            for (int i = 0; i < BUFFERSIZE; ++i) {
                buf.push_back(bFrame());
            }
        };

        void open(string fname) {
            dsmgr.openFile(fname);
        }
        
        int fixPage(int page_id);
        
        bFrame* readPage(int page_id);

        void writePage(int page_id, char* data);

        int UnfixPage(int page_id);

        // returns the number of buffer pages that are free and able to be used. 
        // returns an integer from 0 to BUFFERSIZE-1(1023).
        const int numFreeFrames() {return BUFFERSIZE - frame_num;};

        void incNumFrames() {frame_num += 1;};

        // SelectVictim function selects a frame to replace. 
        virtual int selectVictim() {return 0;};

        void removeBCB(BCB* ptr);

        BCB* addBCB(int page_id, int frame_id); 

        void SetDirty(int frame_id);

        virtual void updateHitted(BCB* bcb) {};

        virtual void updateUnhitted(BCB* bcb) {};

        virtual void printTitle() {
            cout << "##############################" << endl;
            cout << "####### Buffer Manager #######" << endl;
            cout << "##############################" << endl;
        }

        void printStat() {
            printTitle();
            cout << "Total Request : " << access_num<< endl;
            cout << "Total IO : " << dsmgr.getIONum() << endl;
            cout << "Total hit count : " << hit_count << endl;
            cout << "Buffer hit rate : " << (float)(hit_count) / (float) access_num << endl;

        }

    private:
        DSMgr dsmgr;

        int frame_num = 0;

        vector<bFrame> buf;

        // For query BCB via frame id, frame to bcb
        BCB* ftob[BUFFERSIZE];

        unordered_map<int, int> ftop;

        unordered_map<int, BCB*> ptof;

        int hit_count = 0;

        int access_num = 0;
};


class LRUBgr: public Bgr {
    public:
        virtual void updateHitted(BCB* bcb) {
            auto iter = bcb2iter[bcb];
            lru_list.erase(iter);
            lru_list.push_back(bcb);
            bcb2iter[bcb] = --lru_list.end();
        }

        virtual void updateUnhitted(BCB* bcb) {
            lru_list.push_back(bcb);
            bcb2iter[bcb] = --lru_list.end();
        }

        virtual int selectVictim() {
            BCB* bcb = lru_list.front();
            lru_list.pop_front();
            return bcb->frame_id;
        }

        virtual void printTitle() {
            cout << "##############################" << endl;
            cout << "##### LRU Buffer Manager #####" << endl;
            cout << "##############################" << endl;
        }


    private:
        // Used for LRU
        std::list<BCB*> lru_list;

        // Hash map to list iterator for erase
        std::unordered_map<BCB*, list<BCB*>::iterator> bcb2iter;

};

class LRU2Bgr: public Bgr {
    public:
        virtual void updateHitted(BCB* bcb) {
            auto iter = bcb2iter[bcb];
            if (bcb->count < 2) {
                s_lru_list.erase(iter);
            } else {
                l_lru_list.erase(iter);
            }
            bcb->count += 1;
            l_lru_list.push_back(bcb);
            bcb2iter[bcb] = --l_lru_list.end();
        }

        virtual void updateUnhitted(BCB* bcb) {
            bcb->count += 1;
            s_lru_list.push_back(bcb);
            bcb2iter[bcb] = --s_lru_list.end();
        }


        virtual int selectVictim() {
            if (s_lru_list.size() > 0) {
                BCB* bcb = s_lru_list.front();
                s_lru_list.pop_front();
                return bcb->frame_id;
            } else {
                BCB* bcb = l_lru_list.front();
                l_lru_list.pop_front();
                return bcb->frame_id;
            }
        }
        virtual void printTitle() {
            cout << "##############################" << endl;
            cout << "#### LRU2 Buffer Manager #####" << endl;
            cout << "##############################" << endl;
        }

    
    private:
        // access time smaller than k.
        list<BCB*> s_lru_list;

        // access time larger than k.
        list<BCB*> l_lru_list;

        // Hash map to list iterator for erase
        std::unordered_map<BCB*, list<BCB*>::iterator> bcb2iter;

};

class CLOCKBgr: public Bgr {
    public:
        virtual void updateHitted(BCB* bcb) {
            ;
        }

        virtual void updateUnhitted(BCB* bcb) {
            bcb_list.push_back(bcb);
            // Update current
            if (numFreeFrames() > 0) {
                current = --bcb_list.end();
            } else {
                // replaced
                current ++;
                if (current == bcb_list.end()) {
                    current = bcb_list.begin();
                }
            }
        }

        virtual int selectVictim() {
            BCB* bcb = *current;
            while (bcb->referenced) {
                bcb->referenced = false;
                current ++;
                if (current == bcb_list.end()) {
                    current = bcb_list.begin();
                }
            }
            return bcb->frame_id;
        }

        virtual void printTitle() {
            cout << "##############################" << endl;
            cout << "#### CLOCK Buffer Manager ####" << endl;
            cout << "##############################" << endl;
        }


    private:

        list<BCB*>::iterator current;

        list<BCB*> bcb_list;

}; 

class DBSAccesser {
private:
    Bgr* buffer_mgr = NULL;

public:
    DBSAccesser(string filename, string mode="LRU") {
        if (mode == "LRU") {
            buffer_mgr = new LRUBgr();   
        } else if (mode == "LRU2") {
            buffer_mgr = new LRU2Bgr();   
        } else if (mode == "CLOCK") {
            buffer_mgr = new CLOCKBgr();
        }
        buffer_mgr->open(filename);
    };

    virtual ~DBSAccesser() {
        delete buffer_mgr;
    }
    
    bFrame* read(int page_id) {
        return buffer_mgr->readPage(page_id);
    }

    int write(int page_id, char* data) {
        buffer_mgr->writePage(page_id, data);
        return 1;
    }

    void printStat() {
        buffer_mgr->printStat();
    }
};




