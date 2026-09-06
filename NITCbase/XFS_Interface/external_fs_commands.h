#ifndef NITCBASE_EXTERNAL_FS_COMMANDS_H
#define NITCBASE_EXTERNAL_FS_COMMANDS_H

int dump_relcat();
int dump_attrcat();
int dumpBlockAllocationMap();
void ls();
int importRelation(char *fileName);
int exportRelation(char *relname, char *filename);
bool checkIfInvalidCharacter(char character);

#endif //NITCBASE_EXTERNAL_FS_COMMANDS_H
