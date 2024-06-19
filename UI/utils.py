def GetPrivilege(username, ExecCommand):
    dat=ExecCommand(f'query_profile -c {username} -u {username}')
    if dat=='-1':
        return -1
    # dat is something like: <username> <name> <mail> <privilege>
    dat=dat.split(' ')
    return int(dat[3])