#include <stdio.h>
#include <string.h>
#include "PUZZLE.H"
unsigned _stklen=16384;
static int errors;
static FILE *checks;
static void check(int ok,const char *message)
{
    if(!ok){++errors;fprintf(checks,"FAIL: %s\n",message);}
}
static void corrupt(int location,int value)
{
    FILE *source=fopen("PUZZLES.DAT","rb"), *target=fopen("BAD.DAT","wb");
    if(!source || !target){check(0,"Open corruption fixture");return;}
    int byte; long offset=0;
    while((byte=fgetc(source))!=EOF) {fputc(offset==location ? value : byte,target);++offset;}
    fclose(source);fclose(target);
}
int main()
{
    checks=fopen("CHECKS.TXT","w");if(!checks)return 2;
    Puzzle packed, loose;
    for(int id=0;id<=96;++id){
        char filename[24];
        if(!id)strcpy(filename,"SOURCE\\XXX.PUZ");
        else sprintf(filename,"SOURCE\\%03d.PUZ",id);
        check(packed.loadPacked(id),"Packed puzzle loads without loose puzzle directory");
        check(loose.load(filename),"Source puzzle loads");
        check(packed.rows()==loose.rows() && packed.columns()==loose.columns()
            && packed.difficulty()==loose.difficulty() && !strcmp(packed.source(),loose.source()),"All metadata preserved");
        for(int row=0;row<loose.rows();++row)
            for(int col=0;col<loose.columns();++col){
                const Puzzle::Cell *a=packed.cell(row,col), *b=loose.cell(row,col);
                check(a && !memcmp(a,b,sizeof(*b)),"Every solution, clue, hint, lock and initial value preserved");
                if(a && !a->locked)packed.setValue(row,col,a->solution);
            }
        if(id)check(packed.isSolved(),"Numbered packed source solution is valid");
    }
    check(packed.load("puzzles/001.puz"),"Legacy identifier resolves to archive");
    check(packed.load("001.PUZ"),"Bare identifier resolves to archive");
    check(packed.load("xxx.puz") && packed.rows()==9,"Extra puzzle identifier");
    check(packed.loadPacked(1),"Prepare preservation test");
    for(int missing=97;missing<=101;++missing)
        check(!packed.loadPacked(missing) && packed.rows()==6 && packed.cell(1,1)->value==2,
              "Missing archive puzzle preserves board");
    check(!packed.loadPacked(1,"MISSING.DAT"),"Missing archive rejected");
    corrupt(0,0);
    check(!packed.loadPacked(1,"BAD.DAT"),"Bad magic rejected");
    corrupt(10+9+6,2);
    check(!packed.loadPacked(1,"BAD.DAT"),"Unknown compression codec rejected");
    corrupt(10+9+7,0);
    check(!packed.loadPacked(1,"BAD.DAT"),"Checksum mismatch rejected");
    FILE *file=fopen("BAD.DAT","wb");if(!file)return 2;fputs("KAKPUZ1",file);fclose(file);
    check(!packed.loadPacked(1,"BAD.DAT"),"Truncated archive rejected");
    check(packed.rows()==6 && packed.cell(1,1)->value==2,"Corrupt archives preserve previous board");
    remove("BAD.DAT");fclose(checks);
    file=fopen("RESULT.TXT","w");if(!file)return 2;
    fputs(errors ? "Puzzle archive tests failed\n" : "Puzzle archive tests passed\n",file);
    return fclose(file) ? 2 : errors ? 1 : 0;
}
