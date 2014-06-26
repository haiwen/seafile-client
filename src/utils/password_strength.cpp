#include <string>
#include <algorithm>
#include <string.h>
#include "password_strength.h"

using namespace std;


bool duplicateSpaces(char first, char second) {return (first == ' ') && (second = ' '); };

unsigned int getNISTNumBits(const char* passwd, unsigned int len, bool repeat_calc=false)
{
    int result = 0;
    float charmult[256];

    if(repeat_calc)
    {
        result=0;
        for(int i=0;i<256;i++)
            charmult[i]=1;

        char tmp;
        for(int j=0; j<len; j++)
        {
            tmp=passwd[j];
            if( j > 19) result += charmult[tmp];
            else if ( j > 7) result += charmult[tmp]*1.5;
            else if ( j > 0) result += charmult[tmp]*2;
            else result+=4;

            charmult[tmp] *= 0.75;
        }

        return result;
    }

    if (len > 20) return 4 + (7*2) + (12*1.5) + len-20;
    if (len > 8) return 4 + (7*2) + ((len-8) * 1.5);
    if (len > 1) return 4 + ((len-1) * 2);

    return len==1 ? 4 : 0;
}

int isStrongPassword( const char* passwd, unsigned int length,
                      unsigned int minbits=18)
{
    bool upper = false;
    bool lower = false;
    bool numeric = false;
    bool other = false;
    bool space = false;

    int extrabits, numbits, result, numwords;
    extrabits =0;
     numbits = 0;
    result = 0;
    numwords =0;

    char tmp;
    for (int i=0; i<length; i++)
    {
        tmp = passwd[i];
        if ((tmp > 'A') && (tmp<='Z')) upper = true;
        else if ((tmp >= 'a') && (tmp <= 'z')) lower = true;
        else if ((tmp >= '0') && (tmp <= '9')) numeric = true;
        else other = true;
    }

    string passwdStr (passwd);

    if(lower)
    {
        if(upper){
            extrabits += 3;
        }

        if(numeric){
            extrabits += 1;
        }

        if(other){
            extrabits += 2;
        }
    }else{
        if (upper){
            if (numeric){
                extrabits += 1;
            }

            if (other){
                extrabits += 2;
            }

        }else{
            if(numeric){
                extrabits -=4;
            }else{
                extrabits-=2;
            }
    }}


    string passwdTrimmedSpaceStr (passwd);
    string::iterator passIterSpaces = unique(passwdTrimmedSpaceStr.begin(), passwdTrimmedSpaceStr.end(), duplicateSpaces);
    passwdTrimmedSpaceStr.erase(passIterSpaces, passwdTrimmedSpaceStr.end());

    numwords = count(passwdTrimmedSpaceStr.begin(), passwdTrimmedSpaceStr.end(), ' ');

    result = getNISTNumBits(passwd, length, true) + extrabits;

    transform(passwdStr.begin(), passwdStr.end(), passwdStr.begin(), ::tolower);
    string revpasswdStr = string( passwdStr.rbegin(), passwdStr.rend());

    numbits = getNISTNumBits(passwd,length) + extrabits;
    if (result > numbits) result = numbits;

    string qwertystrs[11] = {
    "1234567890-qwertyuiopasdfghjkl;zxcvbnm,./",
    "1qaz2wsx3edc4rfv5tgb6yhn7ujm8ik,9ol.0p;/-['=]:?_{\"+}",
    "1qaz2wsx3edc4rfv5tgb6yhn7ujm8ik9ol0p",
    "qazwsxedcrfvtgbyhnujmik,ol.p;/-['=]:?_{\"+}",
    "qazwsxedcrfvtgbyhnujmikolp",
    "]\"/=[;.-pl,0okm9ijn8uhb7ygv6tfc5rdx4esz3wa2q1",
    "pl0okm9ijn8uhb7ygv6tfc5rdx4esz3wa2q1",
    "]\"/[;.pl,okmijnuhbygvtfcrdxeszwaq",
    "plokmijnuhbygvtfcrdxeszwaq",
    "014725836914702583697894561230258/369*+-*/",
    "abcdefghijklmnopqrstuvwxyz"};

    int z = 6;
    size_t pos;
    for( int i=0; i<10; i++)
    {
        string qpasswdStr = passwdStr;
        string qrevpasswdStr = revpasswdStr;
        do
        {
            int y = qwertystrs[i].length() - z;
            for(int j=0; j<y; j++)
            {
                string str = qwertystrs[i].substr(i, z);

                pos = qpasswdStr.find(str);
                if(pos!=string::npos)
                    qpasswdStr.replace(pos, str.length(), "*");

                pos = qrevpasswdStr.find(str);
                if(pos!=string::npos)
                    qrevpasswdStr.replace(pos, str.length(), "*");

            }

            z--;
        }while(z > 2);

        char* qpasswdCh = new char[qpasswdStr.length()];
        memcpy(qpasswdCh, (char*)qpasswdStr.c_str(), qpasswdStr.length());
        numbits = getNISTNumBits(qpasswdCh, qpasswdStr.size()) + extrabits;
        if (result > numbits) result = numbits;

        char* qrevpasswdCh = new char[qrevpasswdStr.length()];
        memcpy(qrevpasswdCh, (char*)qrevpasswdStr.c_str(), qrevpasswdStr.length());
        numbits = getNISTNumBits(qrevpasswdCh, qrevpasswdStr.size()) + extrabits;
        if (result > numbits) result = numbits;
    }
    return result;
}
