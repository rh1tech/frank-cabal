/* Cabal - Legacy Game Implementations
 *
 * Filesystem implementation for RP2350 using cabal_fs
 */

#include "backends/fs/rp2350/rp2350-fs.h"
#include "cabal_fs.h"
#include "common/textconsole.h"
#include <stdio.h>
#include <string.h>

namespace RP2350 {

//////////////////////////////////////////////////////////////
// RP2350FileSystemNode
//////////////////////////////////////////////////////////////

RP2350FileSystemNode::RP2350FileSystemNode() {
	_displayName = "/";
	_path = "/";
	_isValid = true;
	_isDirectory = true;
}

RP2350FileSystemNode::RP2350FileSystemNode(const Common::String &path) {
	_path = path;

	// Extract display name (last path component)
	if (path == "/" || path.empty()) {
		_displayName = "/";
		_isValid = true;
		_isDirectory = true;
		return;
	}

	// Find last slash
	int lastSlash = -1;
	for (int i = path.size() - 1; i >= 0; i--) {
		if (path[i] == '/' || path[i] == '\\') {
			lastSlash = i;
			break;
		}
	}

	if (lastSlash >= 0 && lastSlash < (int)path.size() - 1) {
		_displayName = Common::String(path.c_str() + lastSlash + 1);
	} else {
		_displayName = path;
	}

	// Check if path exists and whether it's a directory
	_isValid = cabal_path_exists(path.c_str());
	_isDirectory = cabal_path_is_dir(path.c_str());
}

RP2350FileSystemNode::RP2350FileSystemNode(const Common::String &path, bool isDir) {
	_path = path;
	_isDirectory = isDir;
	_isValid = true;

	// Extract display name
	int lastSlash = -1;
	for (int i = path.size() - 1; i >= 0; i--) {
		if (path[i] == '/' || path[i] == '\\') {
			lastSlash = i;
			break;
		}
	}

	if (lastSlash >= 0 && lastSlash < (int)path.size() - 1) {
		_displayName = Common::String(path.c_str() + lastSlash + 1);
	} else {
		_displayName = path;
	}
}

AbstractFSNode *RP2350FileSystemNode::getChild(const Common::String &n) const {
	if (_path.lastChar() == '/' || _path.lastChar() == '\\') {
		return new RP2350FileSystemNode(_path + n);
	} else {
		return new RP2350FileSystemNode(_path + "/" + n);
	}
}

bool RP2350FileSystemNode::getChildren(AbstractFSList &list, ListMode mode, bool hidden) const {
	if (!_isDirectory) {
		return false;
	}

	CabalDir *dir = cabal_dir_open(_path.c_str());
	if (!dir) {
		return false;
	}

	CabalDirEntry entry;
	while (cabal_dir_read(dir, &entry)) {
		// Skip . and ..
		if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) {
			continue;
		}

		// Build full path
		Common::String childPath;
		if (_path.lastChar() == '/' || _path.lastChar() == '\\') {
			childPath = _path + entry.name;
		} else {
			childPath = _path + "/" + entry.name;
		}

		bool isDir = entry.isDirectory;

		// Filter based on mode
		if ((mode == Common::FSNode::kListFilesOnly && isDir) ||
		    (mode == Common::FSNode::kListDirectoriesOnly && !isDir)) {
			continue;
		}

		RP2350FileSystemNode *node = new RP2350FileSystemNode(childPath, isDir);
		list.push_back(node);
	}

	cabal_dir_close(dir);
	return true;
}

AbstractFSNode *RP2350FileSystemNode::getParent() const {
	if (_path == "/" || _path.empty()) {
		return new RP2350FileSystemNode();
	}

	// Find parent path
	int lastSlash = -1;
	for (int i = _path.size() - 1; i >= 0; i--) {
		if (_path[i] == '/' || _path[i] == '\\') {
			lastSlash = i;
			break;
		}
	}

	if (lastSlash <= 0) {
		return new RP2350FileSystemNode();
	}

	Common::String parentPath(_path.c_str(), lastSlash);
	RP2350FileSystemNode *parent = new RP2350FileSystemNode(parentPath);
	parent->_isDirectory = true;
	return parent;
}

Common::SeekableReadStream *RP2350FileSystemNode::createReadStream() {
	return RP2350FileStream::makeFromPath(_path);
}

Common::WriteStream *RP2350FileSystemNode::createWriteStream() {
	// Writing not supported for now
	warning("RP2350FileSystemNode::createWriteStream: Writing not supported");
	return NULL;
}

//////////////////////////////////////////////////////////////
// RP2350FileStream
//////////////////////////////////////////////////////////////

RP2350FileStream::RP2350FileStream(void *handle)
	: _handle(handle), _eos(false), _err(false) {
	// Get file size
	CabalFile *file = (CabalFile *)handle;
	_size = cabal_file_size(file);
}

RP2350FileStream::~RP2350FileStream() {
	if (_handle) {
		cabal_file_close((CabalFile *)_handle);
	}
}

uint32 RP2350FileStream::read(void *dataPtr, uint32 dataSize) {
	if (!_handle || _eos) {
		return 0;
	}

	int32 bytesRead = cabal_file_read((CabalFile *)_handle, dataPtr, dataSize);
	if (bytesRead < 0) {
		_err = true;
		return 0;
	}

	if ((uint32)bytesRead < dataSize) {
		_eos = true;
	}

	return (uint32)bytesRead;
}

int32 RP2350FileStream::pos() const {
	if (!_handle) {
		return -1;
	}
	return cabal_file_tell((CabalFile *)_handle);
}

bool RP2350FileStream::seek(int32 offs, int whence) {
	if (!_handle) {
		return false;
	}

	int32 newPos;
	switch (whence) {
	case SEEK_SET:
		newPos = offs;
		break;
	case SEEK_CUR:
		newPos = cabal_file_tell((CabalFile *)_handle) + offs;
		break;
	case SEEK_END:
		newPos = _size + offs;
		break;
	default:
		return false;
	}

	if (newPos < 0) {
		return false;
	}

	CabalFsResult result = cabal_file_seek((CabalFile *)_handle, newPos);
	if (result == CABAL_FS_OK) {
		_eos = false;  // Clear eos on successful seek
		return true;
	}
	return false;
}

RP2350FileStream *RP2350FileStream::makeFromPath(const Common::String &path) {
	CabalFile *file = cabal_file_open(path.c_str());
	if (!file) {
		return NULL;
	}
	return new RP2350FileStream(file);
}

} // namespace RP2350
