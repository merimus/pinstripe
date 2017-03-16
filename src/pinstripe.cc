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
class Pinstripe {
public:
  static std::string basedir;
  
  static int getattr(const char *path, struct stat *stbuf)
  {
    std::string realpath = basedir + path;
    std::cout << "getattr " << basedir << "," << path << std::endl;
    std::cout << "realpath " << realpath << std::endl;
    int res = lstat(realpath.c_str(), stbuf);
    if (res == -1) {
      perror("getattr");
      return -errno;
    }
    return 0;
  }

  static int mknod(const char* path, mode_t mode, dev_t dev) {
    std::string realpath = basedir + path;
    int res = ::mknod(realpath.c_str(), mode, dev);
    if (res == -1) {
      perror("mknod");
      return -errno;
    }
    return 0;
  }

  static int mkdir(const char *path, mode_t mode) {
    std::string realpath = basedir + path;
    int res = ::mkdir(realpath.c_str(), mode);
    if (res == -1) {
      perror("mkdir");
      return -errno;
    }
    return 0;    
  }

  static int unlink(const char *path) {
    std::string realpath = basedir + path;
    int res = ::unlink(realpath.c_str());
    if (res == -1) {
      perror("unlink");
      return -errno;
    }
    return 0;
  }

  static int rmdir(const char *path) {
    std::string realpath = basedir + path;
    int res = ::rmdir(realpath.c_str());
    if (res == -1) {
      perror("rmdir");
      return -errno;
    }
    return 0;
  }
  
  static int rename(const char *path, const char *newpath) {
    std::string realpath = basedir + path;
    std::string newrealpath = basedir + newpath;
    int res = ::rename(realpath.c_str(), newrealpath.c_str());
    if (res == -1) {
      perror("rename");
      return -errno;
    }
    return 0;
  }

  static int chmod(const char *path, mode_t mode) {
    std::string realpath = basedir + path;
    int res = ::chmod(realpath.c_str(), mode);
    if (res == -1) {
      perror("chmod");
      return -errno;
    }
    return 0;
  }

  static int chown(const char *path, uid_t uid, gid_t gid) {
    std::string realpath = basedir + path;
    int res = ::chown(realpath.c_str(), uid, gid);
    if (res == -1) {
      perror("chown");
      return -errno;
    }
    return 0;
  }

  static int truncate(const char *path, off_t newsize) {
    std::string realpath = basedir + path;
    int res = ::truncate(realpath.c_str(), newsize);
    if (res == -1) {
      perror("truncate");
      return -errno;
    }
    return 0;
  }

  static int utime(const char *path, struct utimbuf *ut) {
    std::string realpath = basedir + path;
    int res = ::utime(realpath.c_str(), ut);
    if (res == -1) {
      perror("utime");
      return -errno;
    }
    return 0;
  }

  static int open(const char *path, struct fuse_file_info *ffi) {
    std::string realpath = basedir + path;
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
    return 0;
  }

  static int write(const char *path, const char *buf, size_t size,
		   off_t offset, struct fuse_file_info *ffi) {
    int res = pwrite(ffi->fh, buf, size, offset);
    if (res == -1) {
      perror("write");
      return -errno;
    }
    return 0;
  }

  static int statfs(const char* path, struct statvfs *statv) {
    std::string realpath = basedir + path;
    int res = statvfs(realpath.c_str(), statv);
    if (res == -1) {
      perror("statfs");
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
    std::string realpath = basedir + path;
    DIR *dir  = ::opendir(realpath.c_str());
    if (dir == 0) {
      perror("statfs");
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

std::string Pinstripe::basedir;

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
  pinstripe_oper.release = Pinstripe::release;
  pinstripe_oper.opendir = Pinstripe::opendir;
  pinstripe_oper.readdir = Pinstripe::readdir;
  pinstripe_oper.releasedir = Pinstripe::releasedir;
  pinstripe_oper.init = Pinstripe::init;
  pinstripe_oper.destroy = Pinstripe::destroy;
  
  ps.basedir = "/home/merimus/fuse/rootdir";
  return fuse_main(argc, argv, &pinstripe_oper, NULL);
}
