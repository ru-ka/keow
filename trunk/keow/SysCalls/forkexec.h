#ifndef FORKEXEC_H
#define FORKEXEC_H

void TransferControlToELFCode();
void ForkChildCopyFromParent();
void SetForkChildContext();
void CopyParentHandlesForExec();

DWORD DoFork(CONTEXT *pCtx);
DWORD DoExecve(const char * filename, char* argv[], char* envp[]);


#endif // FORKEXEC_H
