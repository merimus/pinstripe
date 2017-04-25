#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#include <string>
#include <iostream>

#include "rodsClient.h"
#include "irods_environment_properties.hpp"
#include "phyPathReg.h"
#include "dataObjUnlink.h"
#include "modDataObjMeta.h"

class Pinstripe {
public:
  // The local directory which will be mounted in the overlay.
  static std::string localdir;
  // The path of the directory on the resource server.
  static std::string serverdir;
  // The path under which the file will be registered.
  static std::string regdir;

  static rcComm_t *connection;
 
  static void registerFile(std::string path) {
    dataObjInp_t phyPathRegInp; 
    bzero(&phyPathRegInp, sizeof (phyPathRegInp)); 
    std::string objpath = regdir + path;
    rstrcpy(phyPathRegInp.objPath, objpath.c_str(), MAX_NAME_LEN); 
    std::string filepath = serverdir + path;
    addKeyVal(&phyPathRegInp.condInput, FILE_PATH_KW, filepath.c_str()); 
    addKeyVal(&phyPathRegInp.condInput, FORCE_FLAG_KW, "");
    //    addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, rescource.c_str()); 
    int status = rcPhyPathReg(connection, &phyPathRegInp); 
    if (status < 0) {
      fprintf(stderr, "rcPhyPathReg failed.\n");
      printErrorStack(connection->rError);
    }
  }

  static void unregisterFile(const char *path) {
    std::string objPath = regdir + path;
    dataObjInp_t dataObjInp; 
    bzero(&dataObjInp, sizeof (dataObjInp)); 
    rstrcpy(dataObjInp.objPath, objPath.c_str(), MAX_NAME_LEN);
    dataObjInp.oprType = UNREG_OPR;
    addKeyVal(&dataObjInp.condInput, FORCE_FLAG_KW, ""); 
    int status = rcDataObjUnlink (connection, &dataObjInp); 
    if (status < 0) { 
      fprintf(stderr, "rcDataObjUnlink failed.\n");
      printErrorStack(connection->rError);
    }
  }
 
  static void modMetaData(const char *path, const char* key, const char* val) {
    std::string objPath = regdir + path;
    std::string filePath = serverdir + path;
    dataObjInfo_t objInfo;
    bzero(&objInfo, sizeof(objInfo));
    rstrcpy(objInfo.filePath, filePath.c_str(), MAX_NAME_LEN);
    rstrcpy(objInfo.objPath, objPath.c_str(), MAX_NAME_LEN);

    keyValPair_t keyVal;
    bzero(&keyVal, sizeof(keyValPair_t));
    addKeyVal(&keyVal, key, val);

    modDataObjMeta_t modDataObjMeta;
    bzero(&modDataObjMeta, sizeof(modDataObjMeta_t));
    modDataObjMeta.dataObjInfo = &objInfo;
    modDataObjMeta.regParam = &keyVal;

    int status = rcModDataObjMeta(connection, &modDataObjMeta);
    if (status < 0) {
      fprintf(stderr, "rcModDataObjMeta failed.\n");
      printErrorStack(connection->rError);
    }
  }

  static int getattr(const char *path, struct stat *stbuf)
  {
    std::string realpath = localdir + path;
    std::cout << "getattr " << localdir << "," << path << std::endl;
    std::cout << "realpath " << realpath << std::endl;
    int res = lstat(realpath.c_str(), stbuf);
    if (res == -1) {
      perror("getattr");
      return -errno;
    }
    return 0;
  }

  static int mknod(const char* path, mode_t mode, dev_t dev) {
    std::string realpath = localdir + path;
    int res = ::mknod(realpath.c_str(), mode, dev);
    if (res == -1) {
      perror("mknod");
      return -errno;
    }
    registerFile(path);
    return 0;
  }

  static int mkdir(const char *path, mode_t mode) {
    std::string realpath = localdir + path;
    int res = ::mkdir(realpath.c_str(), mode);
    if (res == -1) {
      perror("mkdir");
      return -errno;
    }
    return 0;    
  }

  static int unlink(const char *path) {
    std::string realpath = localdir + path;
    int res = ::unlink(realpath.c_str());
    if (res == -1) {
      perror("unlink");
      return -errno;
    }
    unregisterFile(path);
    return 0;
  }

  static int rmdir(const char *path) {
    std::string realpath = localdir + path;
    int res = ::rmdir(realpath.c_str());
    if (res == -1) {
      perror("rmdir");
      return -errno;
    }
    return 0;
  }
  
  static int rename(const char *path, const char *newpath) {
    std::string realpath = localdir + path;
    std::string newrealpath = localdir + newpath;
    int res = ::rename(realpath.c_str(), newrealpath.c_str());
    if (res == -1) {
      perror("rename");
      return -errno;
    }
    unregisterFile(path);
    registerFile(newpath);
    return 0;
  }

  static int chmod(const char *path, mode_t mode) {
    std::string realpath = localdir + path;
    int res = ::chmod(realpath.c_str(), mode);
    if (res == -1) {
      perror("chmod");
      return -errno;
    }
    return 0;
  }

  static int chown(const char *path, uid_t uid, gid_t gid) {
    std::string realpath = localdir + path;
    int res = ::chown(realpath.c_str(), uid, gid);
    if (res == -1) {
      perror("chown");
      return -errno;
    }
    return 0;
  }

  static int truncate(const char *path, off_t newsize) {
    std::string realpath = localdir + path;
    int res = ::truncate(realpath.c_str(), newsize);
    if (res == -1) {
      perror("truncate");
      return -errno;
    }
    modMetaData(path, DATA_SIZE_KW, std::to_string(newsize).c_str());
    return 0;
  }

  static int utime(const char *path, struct utimbuf *ut) {
    std::string realpath = localdir + path;
    int res = ::utime(realpath.c_str(), ut);
    if (res == -1) {
      perror("utime");
      return -errno;
    }
    modMetaData(path, DATA_SIZE_KW, std::to_string(ut->modtime).c_str());
    return 0;
  }

  static int open(const char *path, struct fuse_file_info *ffi) {
    std::string realpath = localdir + path;
    int res = ::open(realpath.c_str(), ffi->flags);
    if (res == -1) {
      perror("open");
      return -errno;
    }
    ffi->fh = res;
    return 0;
  }

  static int read(const char *path, char *buf, size_t size, off_t offset,
		  struct fuse_file_info *ffi) {
    int res = pread(ffi->fh, buf, size, offset);
    if (res == -1) {
      perror("read");
      return -errno;
    }
    return res;
  }

  static int write(const char *path, const char *buf, size_t size,
		   off_t offset, struct fuse_file_info *ffi) {
    int res = pwrite(ffi->fh, buf, size, offset);
    if (res == -1) {
      perror("write");
      return -errno;
    }

    struct stat stat_buf;
    if (fstat(ffi->fh, &stat_buf) == -1) {
      perror("fstat");
      return res;
    }
    modMetaData(path, DATA_SIZE_KW, std::to_string(stat_buf.st_size).c_str());
    return res;
  }

  static int statfs(const char* path, struct statvfs *statv) {
    std::string realpath = localdir + path;
    int res = statvfs(realpath.c_str(), statv);
    if (res == -1) {
      perror("statfs");
      return -errno;
    }
    return 0;
  }

  static int flush(const char* path, struct fuse_file_info *ffi) {
    int res = ::close(dup(ffi->fh));
    if (res == -1) {
      perror("flush");
      return -errno;
    }
    registerFile(path);
    return 0;
  }

  static int fsync(const char* path, int ds, struct fuse_file_info *ffi) {
    int res;
    if (ds != 0) {
      res = ::fdatasync(ffi->fh);
    } else {
      res = ::fsync(ffi->fh);
    }
    if (res == -1) {
      perror("fsync");
      return -errno;
    }
    return 0;
  }

  static int release(const char* path, struct fuse_file_info *ffi) {
    int res = close(ffi->fh);
    if (res == -1) {
      perror("statfs");
      return -errno;
    }
    return 0;
  }

  static int opendir(const char *path, struct fuse_file_info *ffi) {
    // We should either mount with default permissions or check perms here.
    std::string realpath = localdir + path;
    DIR *dir  = ::opendir(realpath.c_str());
    if (dir == 0) {
      perror("opendir");
      return -errno;
    }
    ffi->fh = (intptr_t)dir;
    return 0;  
  }

  static int readdir(const char* path, void *buf, fuse_fill_dir_t filler,
		 off_t offset, struct fuse_file_info *ffi) {
    struct dirent *dirent;
    DIR *dir;
    dir = (DIR*)ffi->fh;
    
    dirent = ::readdir(dir);
    if (dirent == 0) {
      perror("readdir");
      return -errno;
    }

    while (dirent != 0) {
      if (filler(buf, dirent->d_name, NULL, 0) != 0) {
	return -ENOMEM;
      }
      dirent = ::readdir(dir);
    }
    return 0;
  }

  static int releasedir(const char *path, struct fuse_file_info *ffi) {
    closedir((DIR*)ffi->fh);
    return 0;
  }

  // access
  // fgetattr

  static void *init(struct fuse_conn_info *conn) {
    return 0;
  }
  static void destroy(void *userdata) {
    return;
  }
};

std::string Pinstripe::localdir;
std::string Pinstripe::serverdir;
std::string Pinstripe::regdir;
rcComm_t *Pinstripe::connection;

int main(int argc, char* argv[]) {
  Pinstripe ps;
  struct fuse_operations pinstripe_oper;
  memset(&pinstripe_oper, 0, sizeof(struct fuse_operations));
  
  pinstripe_oper.getattr = Pinstripe::getattr;
  pinstripe_oper.mknod = Pinstripe::mknod;
  pinstripe_oper.mkdir = Pinstripe::mkdir;
  pinstripe_oper.unlink = Pinstripe::unlink;
  pinstripe_oper.rmdir = Pinstripe::rmdir;
  pinstripe_oper.rename = Pinstripe::rename;
  pinstripe_oper.chmod = Pinstripe::chmod;
  pinstripe_oper.chown = Pinstripe::chown;
  pinstripe_oper.truncate = Pinstripe::truncate;
  pinstripe_oper.utime = Pinstripe::utime;
  pinstripe_oper.open = Pinstripe::open;
  pinstripe_oper.read = Pinstripe::read;
  pinstripe_oper.write = Pinstripe::write;
  pinstripe_oper.statfs = Pinstripe::statfs;
  pinstripe_oper.flush = Pinstripe::flush;
  pinstripe_oper.fsync = Pinstripe::fsync;
  pinstripe_oper.release = Pinstripe::release;
  pinstripe_oper.opendir = Pinstripe::opendir;
  pinstripe_oper.readdir = Pinstripe::readdir;
  pinstripe_oper.releasedir = Pinstripe::releasedir;
  pinstripe_oper.init = Pinstripe::init;
  pinstripe_oper.destroy = Pinstripe::destroy;

  rodsEnv env;
  int status = getRodsEnv(&env);
  if (status < 0) {
    fprintf(stderr, "getRodsEnv failed.\n");
    return 1;
  }
  printRodsEnv(stdout);

  ps.localdir = "/mnt/data";
  ps.serverdir = "/data";
  ps.regdir = "/tempZone/home/merimus";

  /*
  {
    auto &env = irods::environment_properties::instance();
    env.capture();
    std::string source_dir;
    irods::error res = env.get_property("pinstripe_source_dir", source_dir);
    if (!res.ok()) {
      fprintf(stderr, "Failed to get pinstripe_source_dir property.\n");
      return 1;
    }
    ps.basedir = source_dir.c_str();
  }
  */
  
  rErrMsg_t errorMessage;
  ps.connection = rcConnect(env.rodsHost, env.rodsPort, env.rodsUserName,
                	 env.rodsZone, RECONN_TIMEOUT, &errorMessage);
  if (ps.connection == NULL) {
    fprintf(stderr, "Failed in rcConnect: %s", errorMessage.msg);
    return 1;
  }

  status = clientLogin(ps.connection);
  if (status != 0) {
    fprintf(stderr, "clientLogin failed.\n");
    printErrorStack(ps.connection->rError);
    rcDisconnect(ps.connection);
    return 1;
  }

  int res = fuse_main(argc, argv, &pinstripe_oper, NULL);
   
  printErrorStack(ps.connection->rError);
  rcDisconnect(ps.connection);
  return res;
}


 
