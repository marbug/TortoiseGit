// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "TGitPath.h"
#include "Git.h"
#include "StringUtils.h"

extern CGit g_Git;

TEST(CTGitPath, GetDirectoryTest)
{
	// Bit tricky, this test, because we need to know something about the file
	// layout on the machine which is running the test
	TCHAR winDir[MAX_PATH + 1] = { 0 };
	GetWindowsDirectory(winDir, _countof(winDir));
	CString sWinDir(winDir);

	CTGitPath testPath;
	// This is a file which we know will always be there
	testPath.SetFromUnknown(sWinDir + L"\\win.ini");
	EXPECT_FALSE(testPath.IsDirectory());
	EXPECT_STREQ(sWinDir,testPath.GetDirectory().GetWinPathString());
	EXPECT_STREQ(sWinDir, testPath.GetContainingDirectory().GetWinPathString());

	// Now do the test on the win directory itself - It's hard to be sure about the containing directory
	// but we know it must be different to the directory itself
	testPath.SetFromUnknown(sWinDir);
	EXPECT_TRUE(testPath.IsDirectory());
	EXPECT_STREQ(sWinDir, testPath.GetDirectory().GetWinPathString());
	EXPECT_STRNE(sWinDir, testPath.GetContainingDirectory().GetWinPathString());
	EXPECT_GT(sWinDir.GetLength(), testPath.GetContainingDirectory().GetWinPathString().GetLength());

	// Try a root path
	testPath.SetFromUnknown(L"C:\\");
	EXPECT_TRUE(testPath.IsDirectory());
	EXPECT_TRUE(testPath.GetDirectory().GetWinPathString().CompareNoCase(L"C:\\") == 0);
	EXPECT_TRUE(testPath.GetContainingDirectory().IsEmpty());
	// Try a root UNC path
	testPath.SetFromUnknown(L"\\MYSTATION");
	EXPECT_TRUE(testPath.GetContainingDirectory().IsEmpty());

	// test the UI path methods
	testPath.SetFromUnknown(L"c:\\testing%20test");
	EXPECT_TRUE(testPath.GetUIFileOrDirectoryName().CompareNoCase(L"testing%20test") == 0);

	//testPath.SetFromUnknown(L"http://server.com/testing%20special%20chars%20%c3%a4%c3%b6%c3%bc");
	//EXPECT_TRUE(testPath.GetUIFileOrDirectoryName().CompareNoCase(L"testing special chars \344\366\374") == 0);
}

TEST(CTGitPath, AdminDirTest)
{
	CTGitPath testPath;
	testPath.SetFromUnknown(L"c:\\.gitdir");
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\test.git");
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\.git");
	EXPECT_TRUE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\.gitdir\\test");
	EXPECT_FALSE(testPath.IsAdminDir());
	testPath.SetFromUnknown(L"c:\\.git\\test");
	EXPECT_TRUE(testPath.IsAdminDir());

	CTGitPathList pathList;
	pathList.AddPath(CTGitPath(L"c:\\.gitdir"));
	pathList.AddPath(CTGitPath(L"c:\\.git"));
	pathList.AddPath(CTGitPath(L"c:\\.git\\test"));
	pathList.AddPath(CTGitPath(L"c:\\test"));
	pathList.RemoveAdminPaths();
	EXPECT_EQ(2, pathList.GetCount());
	pathList.Clear();
	EXPECT_EQ(0, pathList.GetCount());
	pathList.AddPath(CTGitPath(L"c:\\test"));
	pathList.RemoveAdminPaths();
	EXPECT_EQ(1, pathList.GetCount());
}

TEST(CTGitPath, SortTest)
{
	CTGitPathList testList;
	CTGitPath testPath;
	testPath.SetFromUnknown(L"c:/Z");
	testList.AddPath(testPath);
	testPath.SetFromUnknown(L"c:/B");
	testList.AddPath(testPath);
	testPath.SetFromUnknown(L"c:\\a");
	testList.AddPath(testPath);
	testPath.SetFromUnknown(L"c:/Test");
	testList.AddPath(testPath);

	EXPECT_EQ(4, testList.GetCount());

	testList.SortByPathname();

	EXPECT_EQ(4, testList.GetCount());
	EXPECT_EQ(L"c:\\a", testList[0].GetWinPathString());
	EXPECT_EQ(L"c:\\B", testList[1].GetWinPathString());
	EXPECT_EQ(L"c:\\Test", testList[2].GetWinPathString());
	EXPECT_EQ(L"c:\\Z",testList[3].GetWinPathString());
}

TEST(CTGitPath, RawAppendTest)
{
	CTGitPath testPath(L"c:/test/");
	testPath.AppendRawString(L"/Hello");
	EXPECT_EQ(L"c:\\test\\Hello", testPath.GetWinPathString());

	testPath.AppendRawString(L"\\T2");
	EXPECT_EQ(L"c:\\test\\Hello\\T2", testPath.GetWinPathString());

	CTGitPath testFilePath(L"C:\\windows\\win.ini");
	CTGitPath testBasePath(L"c:/temp/myfile.txt");
	testBasePath.AppendRawString(testFilePath.GetFileExtension());
	EXPECT_EQ(L"c:\\temp\\myfile.txt.ini", testBasePath.GetWinPathString());
}

TEST(CTGitPath, PathAppendTest)
{
	CTGitPath testPath(L"c:/test/");
	testPath.AppendPathString(L"/Hello");
	EXPECT_EQ(L"c:\\test\\Hello", testPath.GetWinPathString());

	testPath.AppendPathString(L"T2");
	EXPECT_EQ(L"c:\\test\\Hello\\T2", testPath.GetWinPathString());

	CTGitPath testFilePath(L"C:\\windows\\win.ini");
	CTGitPath testBasePath(L"c:/temp/myfile.txt");
	// You wouldn't want to do this in real life - you'd use append-raw
	testBasePath.AppendPathString(testFilePath.GetFileExtension());
	EXPECT_EQ(L"c:\\temp\\myfile.txt\\.ini", testBasePath.GetWinPathString());
	EXPECT_EQ(L"c:/temp/myfile.txt/.ini", testBasePath.GetGitPathString());
}

TEST(CTGitPath, RemoveDuplicatesTest)
{
	CTGitPathList list;
	list.AddPath(CTGitPath(L"Z"));
	list.AddPath(CTGitPath(L"A"));
	list.AddPath(CTGitPath(L"E"));
	list.AddPath(CTGitPath(L"E"));

	EXPECT_TRUE(list[2].IsEquivalentTo(list[3]));
	EXPECT_EQ(list[2], list[3]);

	EXPECT_EQ(4, list.GetCount());

	list.RemoveDuplicates();

	EXPECT_EQ(3, list.GetCount());

	EXPECT_STREQ(L"A", list[0].GetWinPathString());
	EXPECT_STREQ(L"E", list[1].GetWinPathString());
	EXPECT_STREQ(L"Z", list[2].GetWinPathString());
}

TEST(CTGitPath, RemoveChildrenTest)
{
	CTGitPathList list;
	list.AddPath(CTGitPath(L"c:\\test"));
	list.AddPath(CTGitPath(L"c:\\test\\file"));
	list.AddPath(CTGitPath(L"c:\\testfile"));
	list.AddPath(CTGitPath(L"c:\\parent"));
	list.AddPath(CTGitPath(L"c:\\parent\\child"));
	list.AddPath(CTGitPath(L"c:\\parent\\child1"));
	list.AddPath(CTGitPath(L"c:\\parent\\child2"));

	EXPECT_EQ(7, list.GetCount());

	list.RemoveChildren();

	EXPECT_EQ(3, list.GetCount());

	list.SortByPathname();

	EXPECT_STREQ(L"c:\\parent", list[0].GetWinPathString());
	EXPECT_STREQ(L"c:\\test", list[1].GetWinPathString());
	EXPECT_STREQ(L"c:\\testfile", list[2].GetWinPathString());
}

TEST(CTGitPath, ContainingDirectoryTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\a\\b\\c\\d\\e");
	CTGitPath dir;
	dir = testPath.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a\\b\\c\\d", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a\\b\\c", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a\\b", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\a", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_STREQ(L"c:\\", dir.GetWinPathString());
	dir = dir.GetContainingDirectory();
	EXPECT_TRUE(dir.IsEmpty());
	EXPECT_STREQ(L"", dir.GetWinPathString());
}

TEST(CTGitPath, AncestorTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\windows");
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(L"c:\\")));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows")));
	EXPECT_FALSE(testPath.IsAncestorOf(CTGitPath(L"c:\\windowsdummy")));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows\\test.txt")));
	EXPECT_TRUE(testPath.IsAncestorOf(CTGitPath(L"c:\\windows\\system32\\test.txt")));
}

/*TEST(CTGitPath, SubversionPathTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\");
	EXPECT_TRUE((testPath.GetGitApiPath(pool), "c:") == 0);
	testPath.SetFromWin(L"c:\\folder");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "c:/folder") == 0);
	testPath.SetFromWin(L"c:\\a\\b\\c\\d\\e");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "c:/a/b/c/d/e") == 0);
	testPath.SetFromUnknown(L"http://testing/");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing") == 0);
	testPath.SetFromGit(NULL);
	EXPECT_TRUE(strlen(testPath.GetGitApiPath(pool)) == 0);

	testPath.SetFromUnknown(L"http://testing again");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20again") == 0);
	testPath.SetFromUnknown(L"http://testing%20again");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20again") == 0);
	testPath.SetFromUnknown(L"http://testing special chars \344\366\374");
	EXPECT_TRUE(strcmp(testPath.GetGitApiPath(pool), "http://testing%20special%20chars%20%c3%a4%c3%b6%c3%bc") == 0);
}*/

TEST(CTGitPath, GetCommonRootTest)
{
	CTGitPath pathA(L"C:\\Development\\LogDlg.cpp");
	CTGitPath pathB(L"C:\\Development\\LogDlg.h");
	CTGitPath pathC(L"C:\\Development\\SomeDir\\LogDlg.h");

	CTGitPathList list;
	list.AddPath(pathA);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development\\LogDlg.cpp") == 0);
	list.AddPath(pathB);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development") == 0);
	list.AddPath(pathC);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"C:\\Development") == 0);

	list.Clear();
	CString sPathList = L"D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup64.wxs*D:\\Development\\StExBar\\StExBar\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup.wxs*D:\\Development\\StExBar\\SKTimeStamp\\src\\setup\\Setup64.wxs";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"D:\\Development\\StExBar") == 0);

	list.Clear();
	sPathList = L"c:\\windows\\explorer.exe*c:\\windows";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows") == 0);

	list.Clear();
	sPathList = L"c:\\windows\\*c:\\windows";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows") == 0);

	list.Clear();
	sPathList = L"c:\\windows\\system32*c:\\windows\\system";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\windows") == 0);

	list.Clear();
	sPathList = L"c:\\windowsdummy*c:\\windows";
	list.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_TRUE(list.GetCommonRoot().GetWinPathString().CompareNoCase(L"c:\\") == 0);
}

TEST(CTGitPath, ValidPathAndUrlTest)
{
	CTGitPath testPath;
	testPath.SetFromWin(L"c:\\a\\b\\c.test.txt");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"D:\\.Net\\SpindleSearch\\");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\test folder\\file");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\folder\\");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\ext.ext.ext\\ext.ext.ext.ext");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\.git");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com\\file");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\test\\conf");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\LPT");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\test\\LPT");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1test");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\?\\c:\\test\\com1test");
	EXPECT_TRUE(testPath.IsValidOnWindows());

	testPath.SetFromWin(L"\\\\Share\\filename");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\filename.extension");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\.git");
	EXPECT_TRUE(testPath.IsValidOnWindows());

	// now the negative tests
	testPath.SetFromWin(L"c:\\test:folder");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\file<name");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\something*else");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\folder\\file?nofile");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\ext.>ension");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1\\filename");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"c:\\com1\\AuX");
	EXPECT_FALSE(testPath.IsValidOnWindows());

	testPath.SetFromWin(L"\\\\Share\\lpt9\\filename");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\prn");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromWin(L"\\\\Share\\NUL");
	EXPECT_FALSE(testPath.IsValidOnWindows());

	// now come some URL tests
	/*testPath.SetFromGit(L"http://myserver.com/repos/trunk");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"https://myserver.com/repos/trunk/file%20with%20spaces");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn://myserver.com/repos/trunk/file with spaces");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn+ssh://www.myserver.com/repos/trunk");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"http://localhost:90/repos/trunk");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"file:///C:/GitRepos/Tester/Proj1/tags/t2");
	EXPECT_TRUE(testPath.IsValidOnWindows());
	// and some negative URL tests
	testPath.SetFromGit(L"httpp://myserver.com/repos/trunk");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"https://myserver.com/rep:os/trunk/file%20with%20spaces");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn://myserver.com/rep<os/trunk/file with spaces");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"svn+ssh://www.myserver.com/repos/trunk/prn/");
	EXPECT_FALSE(testPath.IsValidOnWindows());
	testPath.SetFromGit(L"http://localhost:90/repos/trunk/com1");
	EXPECT_FALSE(testPath.IsValidOnWindows());*/
}

TEST(CTGitPath, ListLoadingTest)
{
	TCHAR buf[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, buf);
	CString sPathList(L"Path1*c:\\path2 with spaces and stuff*\\funnypath\\*");
	CTGitPathList testList;
	testList.LoadFromAsteriskSeparatedString(sPathList);

	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(CString(buf) + L"\\Path1", testList[0].GetWinPathString());
	EXPECT_STREQ(L"c:\\path2 with spaces and stuff", testList[1].GetWinPathString());
	EXPECT_STREQ(L"\\funnypath", testList[2].GetWinPathString());

	EXPECT_STREQ(L"", testList.GetCommonRoot().GetWinPathString());
	testList.Clear();
	sPathList = L"c:\\path2 with spaces and stuff*c:\\funnypath\\*";
	testList.LoadFromAsteriskSeparatedString(sPathList);
	EXPECT_STREQ(L"c:\\", testList.GetCommonRoot().GetWinPathString());
}

TEST(CTGitPath, ParserFromLog_Empty)
{
	CGitByteArray byteArray;
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	EXPECT_EQ(0, testList.GetCount());
}

TEST(CTGitPath, ParserFromLog_Conflict)
{
	// as used in CGit::GetWorkingTreeChanges
	BYTE git_ls_file_u_t_z_output[] = { "M 100644 1f9f46da1ee155aa765d6e379d9d19853358cb07 1	bla.txt\0M 100644 3aa011e7d3609ab9af90c4b10f616312d2be422f 2	bla.txt\0M 100644 56d252d69d535834b9fbfa6f6a633ecd505ea2e6 3	bla.txt\0" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"bla.txt", testList[0].GetGitPathString());
	EXPECT_EQ(0, testList[0].m_Stage); // not set
	EXPECT_EQ(0, testList[0].m_Action);
}

/* git status output for the following tests marked with "(*)"
 * gpl.txt was deleted in index, but still on disk
 * build.txt deleted on disk, but not in index
 * src/Debug-Hints.txt deleted on disk and in index
 * zzz-added-only-in-index-missing-on-fs.txt added in index, and then removed on disk
 * signedness.txt new file
 * renamed release.txt -> release-renamed.txt
 * changed on disk but not in index test/UnitTests/TGitPathTest.cpp

On branch tests
Your branch and 'origin/tests' have diverged,
and have 2 and 1 different commit each, respectively.
(use "git pull" to merge the remote branch into yours)

Changes to be committed:
(use "git reset HEAD <file>..." to unstage)

renamed:    release.txt -> release-renamed.txt
new file:   signedness.txt
deleted:    src/Debug-Hints.txt
deleted:    src/gpl.txt
new file:   zzz-added-only-in-index-missing-on-fs.txt

Changes not staged for commit:
(use "git add/rm <file>..." to update what will be committed)
(use "git checkout -- <file>..." to discard changes in working directory)
(commit or discard the untracked or modified content in submodules)

deleted:    build.txt
modified:   ext/apr (untracked content)
modified:   ext/apr-util (modified content, untracked content)
modified:   ext/libgit2 (new commits)
modified:   test/UnitTests/TGitPathTest.cpp
deleted:    zzz-added-only-in-index-missing-on-fs.txt

Untracked files:
(use "git add <file>..." to include in what will be committed)

src/gpl.txt
*/

TEST(CTGitPath, ParserFromLog_Deleted_From_LsFiles)
{
	// as used in CGit::GetWorkingTreeChanges, based on (*)
	CGitByteArray byteArray;
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray, true));
	ASSERT_EQ(0, testList.GetCount());

	BYTE git_ls_file_d_z_output[] = { "build.txt\0" };
	byteArray.append(git_ls_file_d_z_output, sizeof(git_ls_file_d_z_output));
	EXPECT_EQ(0, testList.ParserFromLog(byteArray, true));
	ASSERT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"build.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, testList[0].m_Action);

	BYTE git_ls_file_d_z_output2[] = { "zzz-added-only-in-index-missing-on-fs.txt\0" };
	byteArray.append(git_ls_file_d_z_output2, sizeof(git_ls_file_d_z_output2));
	EXPECT_EQ(0, testList.ParserFromLog(byteArray, true));
	ASSERT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"build.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, testList[0].m_Action);
	EXPECT_STREQ(L"zzz-added-only-in-index-missing-on-fs.txt", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING, testList[1].m_Action);
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_M_C_z)
{
	// as used in DiffCommand::Execute, based on (*)
	BYTE git_DiffIndex_Raw_M_C_z_output[] = { ":100644 000000 39bba2f577adc09d9706703340daf6e1138cfc71 0000000000000000000000000000000000000000 D\0build.txt\0:160000 160000 6dcb2184662c5b50fd2f6adc63f24398ced2b98c 6dcb2184662c5b50fd2f6adc63f24398ced2b98c M\0ext/apr\0:160000 160000 ca83ff5fbeb9c27454b4f0242f17ebf584ed0521 ca83ff5fbeb9c27454b4f0242f17ebf584ed0521 M\0ext/apr-util\0:160000 160000 a291790a8d42579dafe8684151931847921a9578 0000000000000000000000000000000000000000 M\0ext/libgit2\0:100644 100644 f523fd2eb6ea2328e46d88f6406ccebbcb173c9f f523fd2eb6ea2328e46d88f6406ccebbcb173c9f R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 9c752c94669d3319973c5c363045881d8466af3a A\0signedness.txt\0:100644 000000 53b6e6cefd7fc7d68e5aa73678c2895406614c1e 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:100644 100644 9a59162db1af7a7c7cf7da0417b89065ac71b987 0000000000000000000000000000000000000000 M\0test/UnitTests/TGitPathTest.cpp\0" };
	// gpl.txt still exists, but deleted in index
	// build.txt deleted, still in index
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_M_C_z_output, sizeof(git_DiffIndex_Raw_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(9, testList.GetCount());
	EXPECT_STREQ(L"build.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[0].m_Action);
	EXPECT_STREQ(L"", testList[0].m_StatAdd);
	EXPECT_STREQ(L"", testList[0].m_StatDel);
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_STREQ(L"ext/apr", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[1].m_Action);
	EXPECT_STREQ(L"", testList[1].m_StatAdd);
	EXPECT_STREQ(L"", testList[1].m_StatDel);
	EXPECT_EQ(0, testList[1].m_Stage);
	EXPECT_STREQ(L"ext/apr-util", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[2].m_Action);
	EXPECT_STREQ(L"", testList[2].m_StatAdd);
	EXPECT_STREQ(L"", testList[2].m_StatDel);
	EXPECT_EQ(0, testList[2].m_Stage);
	EXPECT_STREQ(L"ext/libgit2", testList[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[3].m_Action);
	EXPECT_STREQ(L"", testList[3].m_StatAdd);
	EXPECT_STREQ(L"", testList[3].m_StatDel);
	EXPECT_EQ(0, testList[3].m_Stage);
	EXPECT_STREQ(L"release-renamed.txt", testList[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[4].m_Action);
	EXPECT_STREQ(L"", testList[4].m_StatAdd);
	EXPECT_STREQ(L"", testList[4].m_StatDel);
	EXPECT_EQ(0, testList[4].m_Stage);
	EXPECT_STREQ(L"release.txt", testList[4].GetGitOldPathString());
	EXPECT_STREQ(L"signedness.txt", testList[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[5].m_Action);
	EXPECT_STREQ(L"", testList[5].m_StatAdd);
	EXPECT_STREQ(L"", testList[5].m_StatDel);
	EXPECT_EQ(0, testList[5].m_Stage);
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[6].m_Action);
	EXPECT_STREQ(L"", testList[6].m_StatAdd);
	EXPECT_STREQ(L"", testList[6].m_StatDel);
	EXPECT_EQ(0, testList[6].m_Stage);
	EXPECT_STREQ(L"src/gpl.txt", testList[7].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[7].m_Action);
	EXPECT_STREQ(L"", testList[7].m_StatAdd);
	EXPECT_STREQ(L"", testList[7].m_StatDel);
	EXPECT_EQ(0, testList[7].m_Stage);
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[8].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[8].m_Action);
	EXPECT_STREQ(L"", testList[8].m_StatAdd);
	EXPECT_STREQ(L"", testList[8].m_StatDel);
	EXPECT_EQ(0, testList[8].m_Stage);
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_M_C_Numstat_z)
{
	// as used in CGit::GetWorkingTreeChanges, but here separated; based on (*)
	BYTE git_DiffIndex_Raw_M_C_z_output[] = { ":100644 000000 39bba2f577adc09d9706703340daf6e1138cfc71 0000000000000000000000000000000000000000 D\0build.txt\0:160000 160000 6dcb2184662c5b50fd2f6adc63f24398ced2b98c 6dcb2184662c5b50fd2f6adc63f24398ced2b98c M\0ext/apr\0:160000 160000 ca83ff5fbeb9c27454b4f0242f17ebf584ed0521 ca83ff5fbeb9c27454b4f0242f17ebf584ed0521 M\0ext/apr-util\0:160000 160000 a291790a8d42579dafe8684151931847921a9578 0000000000000000000000000000000000000000 M\0ext/libgit2\0:100644 100644 f523fd2eb6ea2328e46d88f6406ccebbcb173c9f f523fd2eb6ea2328e46d88f6406ccebbcb173c9f R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 9c752c94669d3319973c5c363045881d8466af3a A\0signedness.txt\0:100644 000000 53b6e6cefd7fc7d68e5aa73678c2895406614c1e 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:100644 100644 9a59162db1af7a7c7cf7da0417b89065ac71b987 0000000000000000000000000000000000000000 M\0test/UnitTests/TGitPathTest.cpp\0""0	61	build.txt\0""0	0	ext/apr\0""0	0	ext/apr-util\0""1	1	ext/libgit2\0""0	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	74	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""162	2	test/UnitTests/TGitPathTest.cpp\0" };
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_M_C_z_output, sizeof(git_DiffIndex_Raw_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(9, testList.GetCount());
	EXPECT_STREQ(L"build.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[0].m_Action);
	EXPECT_STREQ(L"0", testList[0].m_StatAdd);
	EXPECT_STREQ(L"61", testList[0].m_StatDel);
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_STREQ(L"ext/apr", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[1].m_Action);
	EXPECT_STREQ(L"0", testList[1].m_StatAdd);
	EXPECT_STREQ(L"0", testList[1].m_StatDel);
	EXPECT_EQ(0, testList[1].m_Stage);
	EXPECT_STREQ(L"ext/apr-util", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[2].m_Action);
	EXPECT_STREQ(L"0", testList[2].m_StatAdd);
	EXPECT_STREQ(L"0", testList[2].m_StatDel);
	EXPECT_EQ(0, testList[2].m_Stage);
	EXPECT_STREQ(L"ext/libgit2", testList[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[3].m_Action);
	EXPECT_STREQ(L"1", testList[3].m_StatAdd);
	EXPECT_STREQ(L"1", testList[3].m_StatDel);
	EXPECT_EQ(0, testList[3].m_Stage);
	EXPECT_STREQ(L"release-renamed.txt", testList[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[4].m_Action);
	EXPECT_STREQ(L"0", testList[4].m_StatAdd);
	EXPECT_STREQ(L"0", testList[4].m_StatDel);
	EXPECT_EQ(0, testList[4].m_Stage);
	EXPECT_STREQ(L"release.txt", testList[4].GetGitOldPathString());
	EXPECT_STREQ(L"signedness.txt", testList[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[5].m_Action);
	EXPECT_STREQ(L"1176", testList[5].m_StatAdd);
	EXPECT_STREQ(L"0", testList[5].m_StatDel);
	EXPECT_EQ(0, testList[5].m_Stage);
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[6].m_Action);
	EXPECT_STREQ(L"0", testList[6].m_StatAdd);
	EXPECT_STREQ(L"74", testList[6].m_StatDel);
	EXPECT_EQ(0, testList[6].m_Stage);
	EXPECT_STREQ(L"src/gpl.txt", testList[7].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[7].m_Action);
	EXPECT_STREQ(L"0", testList[7].m_StatAdd);
	EXPECT_STREQ(L"340", testList[7].m_StatDel);
	EXPECT_EQ(0, testList[7].m_Stage);
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[8].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[8].m_Action);
	EXPECT_STREQ(L"162", testList[8].m_StatAdd);
	EXPECT_STREQ(L"2", testList[8].m_StatDel);
	EXPECT_EQ(0, testList[8].m_Stage);
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_Cached_M_C_Numstat_z)
{
	// as used in CGit::GetWorkingTreeChanges, but here separated; based on (*)
	BYTE git_DiffIndex_Raw_Cached_M_C_z_output[] = { ":100644 100644 f523fd2eb6ea2328e46d88f6406ccebbcb173c9f f523fd2eb6ea2328e46d88f6406ccebbcb173c9f R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 9c752c94669d3319973c5c363045881d8466af3a A\0signedness.txt\0:100644 000000 53b6e6cefd7fc7d68e5aa73678c2895406614c1e 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:000000 100644 0000000000000000000000000000000000000000 285fc7c8fe9f37876efe56dbb31e34da8b75be24 A\0zzz-added-only-in-index-missing-on-fs.txt\0""0	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	74	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""1	0	zzz-added-only-in-index-missing-on-fs.txt\0" };
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_Cached_M_C_z_output, sizeof(git_DiffIndex_Raw_Cached_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(5, testList.GetCount());
	EXPECT_STREQ(L"release-renamed.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[0].m_Action);
	EXPECT_STREQ(L"0", testList[0].m_StatAdd);
	EXPECT_STREQ(L"0", testList[0].m_StatDel);
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_STREQ(L"release.txt", testList[0].GetGitOldPathString());
	EXPECT_STREQ(L"signedness.txt", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[1].m_Action);
	EXPECT_STREQ(L"1176", testList[1].m_StatAdd);
	EXPECT_STREQ(L"0", testList[1].m_StatDel);
	EXPECT_EQ(0, testList[1].m_Stage);
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[2].m_Action);
	EXPECT_STREQ(L"0", testList[2].m_StatAdd);
	EXPECT_STREQ(L"74", testList[2].m_StatDel);
	EXPECT_EQ(0, testList[2].m_Stage);
	EXPECT_STREQ(L"src/gpl.txt", testList[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[3].m_Action);
	EXPECT_STREQ(L"0", testList[3].m_StatAdd);
	EXPECT_STREQ(L"340", testList[3].m_StatDel);
	EXPECT_EQ(0, testList[3].m_Stage);
	EXPECT_STREQ(L"zzz-added-only-in-index-missing-on-fs.txt", testList[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[4].m_Action);
	EXPECT_STREQ(L"1", testList[4].m_StatAdd);
	EXPECT_STREQ(L"0", testList[4].m_StatDel);
	EXPECT_EQ(0, testList[4].m_Stage);
}

TEST(CTGitPath, ParserFromLog_DiffIndex_Raw_Cached_M_C_Numstat_z_AND_DiffIndex_Raw_M_C_Numstat_z)
{
	// as used in CGit::GetWorkingTreeChanges; based on (*)
	BYTE git_DiffIndex_Raw_Cached_M_C_z_output[] = { ":100644 100644 f523fd2eb6ea2328e46d88f6406ccebbcb173c9f f523fd2eb6ea2328e46d88f6406ccebbcb173c9f R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 9c752c94669d3319973c5c363045881d8466af3a A\0signedness.txt\0:100644 000000 53b6e6cefd7fc7d68e5aa73678c2895406614c1e 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:000000 100644 0000000000000000000000000000000000000000 285fc7c8fe9f37876efe56dbb31e34da8b75be24 A\0zzz-added-only-in-index-missing-on-fs.txt\0""0	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	74	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""1	0	zzz-added-only-in-index-missing-on-fs.txt\0:100644 000000 39bba2f577adc09d9706703340daf6e1138cfc71 0000000000000000000000000000000000000000 D\0build.txt\0:160000 160000 6dcb2184662c5b50fd2f6adc63f24398ced2b98c 6dcb2184662c5b50fd2f6adc63f24398ced2b98c M\0ext/apr\0:160000 160000 ca83ff5fbeb9c27454b4f0242f17ebf584ed0521 ca83ff5fbeb9c27454b4f0242f17ebf584ed0521 M\0ext/apr-util\0:160000 160000 a291790a8d42579dafe8684151931847921a9578 0000000000000000000000000000000000000000 M\0ext/libgit2\0:100644 100644 f523fd2eb6ea2328e46d88f6406ccebbcb173c9f f523fd2eb6ea2328e46d88f6406ccebbcb173c9f R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000000000000000000000000000000000000 9c752c94669d3319973c5c363045881d8466af3a A\0signedness.txt\0:100644 000000 53b6e6cefd7fc7d68e5aa73678c2895406614c1e 0000000000000000000000000000000000000000 D\0src/Debug-Hints.txt\0:100644 000000 36488d58658d63f9d7a8d5e0637c530df987c4e5 0000000000000000000000000000000000000000 D\0src/gpl.txt\0:100644 100644 9a59162db1af7a7c7cf7da0417b89065ac71b987 0000000000000000000000000000000000000000 M\0test/UnitTests/TGitPathTest.cpp\0""0	61	build.txt\0""0	0	ext/apr\0""0	0	ext/apr-util\0""1	1	ext/libgit2\0""0	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	74	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""162	2	test/UnitTests/TGitPathTest.cpp\0" };
	CGitByteArray byteArray;
	byteArray.append(git_DiffIndex_Raw_Cached_M_C_z_output, sizeof(git_DiffIndex_Raw_Cached_M_C_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(10, testList.GetCount());
	EXPECT_STREQ(L"release-renamed.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[0].m_Action);
	EXPECT_STREQ(L"release.txt", testList[0].GetGitOldPathString());
	EXPECT_STREQ(L"0", testList[0].m_StatAdd);
	EXPECT_STREQ(L"0", testList[0].m_StatDel);
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_STREQ(L"signedness.txt", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[1].m_Action);
	EXPECT_STREQ(L"1176", testList[1].m_StatAdd);
	EXPECT_STREQ(L"0", testList[1].m_StatDel);
	EXPECT_EQ(0, testList[1].m_Stage);
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[2].m_Action);
	EXPECT_STREQ(L"0", testList[2].m_StatAdd);
	EXPECT_STREQ(L"74", testList[2].m_StatDel);
	EXPECT_EQ(0, testList[2].m_Stage);
	EXPECT_STREQ(L"src/gpl.txt", testList[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[3].m_Action);
	EXPECT_STREQ(L"0", testList[3].m_StatAdd);
	EXPECT_STREQ(L"340", testList[3].m_StatDel);
	EXPECT_EQ(0, testList[3].m_Stage);
	EXPECT_STREQ(L"zzz-added-only-in-index-missing-on-fs.txt", testList[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[4].m_Action);
	EXPECT_STREQ(L"1", testList[4].m_StatAdd);
	EXPECT_STREQ(L"0", testList[4].m_StatDel);
	EXPECT_EQ(0, testList[4].m_Stage);
	EXPECT_STREQ(L"build.txt", testList[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[5].m_Action);
	EXPECT_STREQ(L"0", testList[5].m_StatAdd);
	EXPECT_STREQ(L"61", testList[5].m_StatDel);
	EXPECT_EQ(0, testList[5].m_Stage);
	EXPECT_STREQ(L"ext/apr", testList[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[6].m_Action);
	EXPECT_STREQ(L"0", testList[6].m_StatAdd);
	EXPECT_STREQ(L"0", testList[6].m_StatDel);
	EXPECT_EQ(0, testList[6].m_Stage);
	EXPECT_STREQ(L"ext/apr-util", testList[7].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[7].m_Action);
	EXPECT_STREQ(L"0", testList[7].m_StatAdd);
	EXPECT_STREQ(L"0", testList[7].m_StatDel);
	EXPECT_EQ(0, testList[7].m_Stage);
	EXPECT_STREQ(L"ext/libgit2", testList[8].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[8].m_Action);
	EXPECT_STREQ(L"1", testList[8].m_StatAdd);
	EXPECT_STREQ(L"1", testList[8].m_StatDel);
	EXPECT_EQ(0, testList[8].m_Stage);
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[9].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[9].m_Action);
	EXPECT_STREQ(L"162", testList[9].m_StatAdd);
	EXPECT_STREQ(L"2", testList[9].m_StatDel);
	EXPECT_EQ(0, testList[9].m_Stage);
}

TEST(CTGitPath, ParserFromLog_Diff_r_raw_C_M_numstat_z_HEAD)
{
	// based on (*)
	BYTE git_Diff_r_raw_C_M_numstat_z_HEAD_output[] = { ":100644 000000 39bba2f... 0000000... D\0build.txt\0:160000 160000 6dcb218... 6dcb218... M\0ext/apr\0:160000 160000 ca83ff5... ca83ff5... M\0ext/apr-util\0:160000 160000 a291790... 0000000... M\0ext/libgit2\0:100644 100644 f523fd2... f523fd2... R100\0release.txt\0release-renamed.txt\0:000000 100644 0000000... 9c752c9... A\0signedness.txt\0:100644 000000 53b6e6c... 0000000... D\0src/Debug-Hints.txt\0:100644 000000 36488d5... 0000000... D\0src/gpl.txt\0:100644 100644 9a59162... 0000000... M\0test/UnitTests/TGitPathTest.cpp\0""0	61	build.txt\0""0	0	ext/apr\0""0	0	ext/apr-util\0""1	1	ext/libgit2\0""0	0	\0release.txt\0release-renamed.txt\0""1176	0	signedness.txt\0""0	74	src/Debug-Hints.txt\0""0	340	src/gpl.txt\0""162	2	test/UnitTests/TGitPathTest.cpp\0" };
	CGitByteArray byteArray;
	byteArray.append(git_Diff_r_raw_C_M_numstat_z_HEAD_output, sizeof(git_Diff_r_raw_C_M_numstat_z_HEAD_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(9, testList.GetCount());
	EXPECT_STREQ(L"build.txt", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[0].m_Action);
	EXPECT_STREQ(L"0", testList[0].m_StatAdd);
	EXPECT_STREQ(L"61", testList[0].m_StatDel);
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_STREQ(L"ext/apr", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[1].m_Action);
	EXPECT_STREQ(L"0", testList[1].m_StatAdd);
	EXPECT_STREQ(L"0", testList[1].m_StatDel);
	EXPECT_EQ(0, testList[1].m_Stage);
	EXPECT_STREQ(L"ext/apr-util", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[2].m_Action);
	EXPECT_STREQ(L"0", testList[2].m_StatAdd);
	EXPECT_STREQ(L"0", testList[2].m_StatDel);
	EXPECT_EQ(0, testList[2].m_Stage);
	EXPECT_STREQ(L"ext/libgit2", testList[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[3].m_Action);
	EXPECT_STREQ(L"1", testList[3].m_StatAdd);
	EXPECT_STREQ(L"1", testList[3].m_StatDel);
	EXPECT_EQ(0, testList[3].m_Stage);
	EXPECT_STREQ(L"release-renamed.txt", testList[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[4].m_Action);
	EXPECT_STREQ(L"0", testList[4].m_StatAdd);
	EXPECT_STREQ(L"0", testList[4].m_StatDel);
	EXPECT_EQ(0, testList[4].m_Stage);
	EXPECT_STREQ(L"release.txt", testList[4].GetGitOldPathString());
	EXPECT_STREQ(L"signedness.txt", testList[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_ADDED, testList[5].m_Action);
	EXPECT_STREQ(L"1176", testList[5].m_StatAdd);
	EXPECT_STREQ(L"0", testList[5].m_StatDel);
	EXPECT_EQ(0, testList[5].m_Stage);
	EXPECT_STREQ(L"src/Debug-Hints.txt", testList[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[6].m_Action);
	EXPECT_STREQ(L"0", testList[6].m_StatAdd);
	EXPECT_STREQ(L"74", testList[6].m_StatDel);
	EXPECT_EQ(0, testList[6].m_Stage);
	EXPECT_STREQ(L"src/gpl.txt", testList[7].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[7].m_Action);
	EXPECT_STREQ(L"0", testList[7].m_StatAdd);
	EXPECT_STREQ(L"340", testList[7].m_StatDel);
	EXPECT_EQ(0, testList[7].m_Stage);
	EXPECT_STREQ(L"test/UnitTests/TGitPathTest.cpp", testList[8].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[8].m_Action);
	EXPECT_STREQ(L"162", testList[8].m_StatAdd);
	EXPECT_STREQ(L"2", testList[8].m_StatDel);
	EXPECT_EQ(0, testList[8].m_Stage);
}

TEST(CTGitPath, ParserFromLog_DiffTree)
{
	// git.exe diff-tree -r --raw -C -M --numstat -z 8a75b51cc7f10b9755fc89837fe78b0c646b8b12~1 8a75b51cc7f10b9755fc89837fe78b0c646b8b12
	BYTE git_difftree_output[] = { ":100644 100644 748cb10e524d745ccd55bdeff67a00bed35144f5 d8899ee55a66b2bfa7f497bfef90e951d846de03 M\0src/Git/Git.cpp\0:100644 100644 ca24882eb4ef523381c2a179697d0f0e069c4f85 569c603dae424b8dda6726f8fb6157bd6e527d9a M\0src/Git/Git.h\0:100644 100644 575645aee2c712ee92f31b6a57d594297e2e5370 71bff876b3465c6eb621f3f3ecb0657e4a8764d7 M\0src/Git/Git.vcxproj\0:100644 100644 d5a11fbc6da73c8070a5359c78d45075fadba6eb 7284c5914b55eae97078df4a97f9ec791446133d M\0src/Git/Git.vcxproj.filters\0:100644 000000 c298e597d65a049782f03e197d654c01579b4990 0000000000000000000000000000000000000000 D\0src/Git/GitConfig.cpp\0:100644 100644 0667abe736182c63d2f2e6c0f29d85e1d9fd0775 a58a0cd9e08d9d849387fdfa2adfc3066cbb85df R081\0src/Git/GitConfig.h\0src/Git/GitForWindows.h\0:100644 100644 582db6c063e34903ee1348bdffbb48dfe484f8e4 ec5178786fcf587fdd6b19349d53c22f1dbcdfbe M\0src/Git/GitIndex.cpp\0:100644 100644 e4d6608dcb8fe635515f42ed9c9e6fdc1fc6c216 4c0e326cda7a479e02a650367561a8b79a3082f2 M\0src/TortoiseProc/Settings/SetMainPage.cpp\0:100644 100644 52aa2c94e5cfa9a5287c84b20687b3365836f1f8 7eb74479bbc3a2e2089a5d32acdd915aacc0f40b M\0src/TortoiseProc/TortoiseProc.cpp\0""1	1	src/Git/Git.cpp\0""3	0	src/Git/Git.h\0""0	2	src/Git/Git.vcxproj\0""0	6	src/Git/Git.vcxproj.filters\0""0	29	src/Git/GitConfig.cpp\0""1	11	\0src/Git/GitConfig.h\0src/Git/GitForWindows.h\0""0	1	src/Git/GitIndex.cpp\0""1	1	src/TortoiseProc/Settings/SetMainPage.cpp\0""0	1	src/TortoiseProc/TortoiseProc.cpp\0" };
	CGitByteArray byteArray;
	byteArray.append(git_difftree_output, sizeof(git_difftree_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLog(byteArray));
	ASSERT_EQ(9, testList.GetCount());
	EXPECT_STREQ(L"src/Git/Git.cpp", testList[0].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[0].m_Action);
	EXPECT_STREQ(L"1", testList[0].m_StatAdd);
	EXPECT_STREQ(L"1", testList[0].m_StatDel);
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_STREQ(L"src/Git/Git.h", testList[1].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[1].m_Action);
	EXPECT_STREQ(L"3", testList[1].m_StatAdd);
	EXPECT_STREQ(L"0", testList[1].m_StatDel);
	EXPECT_EQ(0, testList[1].m_Stage);
	EXPECT_STREQ(L"src/Git/Git.vcxproj", testList[2].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[2].m_Action);
	EXPECT_STREQ(L"0", testList[2].m_StatAdd);
	EXPECT_STREQ(L"2", testList[2].m_StatDel);
	EXPECT_EQ(0, testList[2].m_Stage);
	EXPECT_STREQ(L"src/Git/Git.vcxproj.filters", testList[3].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[3].m_Action);
	EXPECT_STREQ(L"0", testList[3].m_StatAdd);
	EXPECT_STREQ(L"6", testList[3].m_StatDel);
	EXPECT_EQ(0, testList[3].m_Stage);
	EXPECT_STREQ(L"src/Git/GitConfig.cpp", testList[4].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_DELETED, testList[4].m_Action);
	EXPECT_STREQ(L"0", testList[4].m_StatAdd);
	EXPECT_STREQ(L"29", testList[4].m_StatDel);
	EXPECT_EQ(0, testList[4].m_Stage);
	EXPECT_STREQ(L"src/Git/GitForWindows.h", testList[5].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_REPLACED, testList[5].m_Action);
	EXPECT_STREQ(L"src/Git/GitConfig.h", testList[5].GetGitOldPathString());
	EXPECT_STREQ(L"1", testList[5].m_StatAdd);
	EXPECT_STREQ(L"11", testList[5].m_StatDel);
	EXPECT_EQ(0, testList[5].m_Stage);
	EXPECT_STREQ(L"src/Git/GitIndex.cpp", testList[6].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[6].m_Action);
	EXPECT_STREQ(L"0", testList[6].m_StatAdd);
	EXPECT_STREQ(L"1", testList[6].m_StatDel);
	EXPECT_EQ(0, testList[6].m_Stage);
	EXPECT_STREQ(L"src/TortoiseProc/Settings/SetMainPage.cpp", testList[7].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[7].m_Action);
	EXPECT_STREQ(L"1", testList[7].m_StatAdd);
	EXPECT_STREQ(L"1", testList[7].m_StatDel);
	EXPECT_EQ(0, testList[7].m_Stage);
	EXPECT_STREQ(L"src/TortoiseProc/TortoiseProc.cpp", testList[8].GetGitPathString());
	EXPECT_EQ(CTGitPath::LOGACTIONS_MODIFIED, testList[8].m_Action);
	EXPECT_STREQ(L"0", testList[8].m_StatAdd);
	EXPECT_STREQ(L"1", testList[8].m_StatDel);
	EXPECT_EQ(0, testList[8].m_Stage);
}

static void setFlagOnFileInIndex(CAutoIndex& gitindex, const CString& filename, bool assumevalid, bool skipworktree)
{
	size_t idx;
	EXPECT_TRUE(git_index_find(&idx, gitindex, CUnicodeUtils::GetUTF8(filename)) == 0);
	git_index_entry *e = const_cast<git_index_entry *>(git_index_get_byindex(gitindex, idx));
	ASSERT_TRUE(e);
	if (assumevalid == BST_UNCHECKED)
		e->flags &= ~GIT_IDXENTRY_VALID;
	else if (assumevalid == BST_CHECKED)
		e->flags |= GIT_IDXENTRY_VALID;
	if (skipworktree == BST_UNCHECKED)
		e->flags_extended &= ~GIT_IDXENTRY_SKIP_WORKTREE;
	else if (skipworktree == BST_CHECKED)
		e->flags_extended |= GIT_IDXENTRY_SKIP_WORKTREE;
	EXPECT_TRUE(git_index_add(gitindex, e) == 0);
}

TEST(CTGitPath, FillBasedOnIndexFlags)
{
	CAutoTempDir tmpDir;

	CString output;
	CAutoRepository repo = nullptr;
	EXPECT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), 0) == 0);
	
	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\a", nullptr));
	ASSERT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\b", nullptr));

	CAutoIndex gitindex;
	EXPECT_TRUE(git_repository_index(gitindex.GetPointer(), repo) == 0);

	CString filenames[] = { L"versioned", L"assume-unchanged", L"skip-worktree", L"unversioned", L"a/versioned", L"a/assume-unchanged", L"a/skip-worktree", L"a/unversioned", L"b/versioned", L"b/assume-unchanged", L"b/skip-worktree", L"b/unversioned" };
	for (const CString& filename : filenames)
	{
		CString filenameWithPath = tmpDir.GetTempDir() + L'\\' + filename;
		EXPECT_TRUE(CStringUtils::WriteStringToTextFile(filenameWithPath, L"something"));
		EXPECT_TRUE(git_index_add_bypath(gitindex, CUnicodeUtils::GetUTF8(filename)) == 0);
	}

	setFlagOnFileInIndex(gitindex, L"assume-unchanged", true, false);
	setFlagOnFileInIndex(gitindex, L"a/assume-unchanged", true, false);
	setFlagOnFileInIndex(gitindex, L"b/assume-unchanged", true, false);
	setFlagOnFileInIndex(gitindex, L"skip-worktree", false, true);
	setFlagOnFileInIndex(gitindex, L"a/skip-worktree", false, true);
	setFlagOnFileInIndex(gitindex, L"b/skip-worktree", false, true);

	EXPECT_TRUE(git_index_write(gitindex) == 0);
	gitindex.Free();
	repo.Free();

	g_Git.m_CurrentDir = tmpDir.GetTempDir();

	CTGitPathList testList;
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(0, 0) == 0);
	EXPECT_EQ(0, testList.GetCount());

	testList.Clear();
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_IDXENTRY_VALID, 0) == 0);
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
	EXPECT_STREQ(L"assume-unchanged", testList[1].GetGitPathString());
	EXPECT_STREQ(L"b/assume-unchanged", testList[2].GetGitPathString());

	testList.Clear();
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(0, GIT_IDXENTRY_SKIP_WORKTREE) == 0);
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"a/skip-worktree", testList[0].GetGitPathString());
	EXPECT_STREQ(L"b/skip-worktree", testList[1].GetGitPathString());
	EXPECT_STREQ(L"skip-worktree", testList[2].GetGitPathString());
	
	testList.Clear();
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_IDXENTRY_VALID, GIT_IDXENTRY_SKIP_WORKTREE) == 0);
	EXPECT_EQ(6, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
	EXPECT_STREQ(L"a/skip-worktree", testList[1].GetGitPathString());
	EXPECT_STREQ(L"assume-unchanged", testList[2].GetGitPathString());
	EXPECT_STREQ(L"b/assume-unchanged", testList[3].GetGitPathString());
	EXPECT_STREQ(L"b/skip-worktree", testList[4].GetGitPathString());
	EXPECT_STREQ(L"skip-worktree", testList[5].GetGitPathString());

	CTGitPathList selectList;
	selectList.AddPath(CTGitPath(L"versioned"));
	selectList.AddPath(CTGitPath(L"assume-unchanged"));
	selectList.AddPath(CTGitPath(L"skip-worktree"));
	selectList.AddPath(CTGitPath(L"a/skip-worktree"));
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_IDXENTRY_VALID, GIT_IDXENTRY_SKIP_WORKTREE, &selectList) == 0);
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"a/skip-worktree", testList[0].GetGitPathString());
	EXPECT_STREQ(L"assume-unchanged", testList[1].GetGitPathString());
	EXPECT_STREQ(L"skip-worktree", testList[2].GetGitPathString());

	selectList.Clear();
	selectList.AddPath(CTGitPath(L"a"));
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_IDXENTRY_VALID, GIT_IDXENTRY_SKIP_WORKTREE, &selectList) == 0);
	EXPECT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
	EXPECT_STREQ(L"a/skip-worktree", testList[1].GetGitPathString());

	selectList.Clear();
	selectList.AddPath(CTGitPath(L"a"));
	EXPECT_TRUE(testList.FillBasedOnIndexFlags(GIT_IDXENTRY_VALID, 0, &selectList) == 0);
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"a/assume-unchanged", testList[0].GetGitPathString());
}

TEST(CTGitPath, ParserFromLsFile_Empty)
{
	CGitByteArray byteArray;
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray));
	EXPECT_EQ(0, testList.GetCount());
}

TEST(CTGitPath, ParserFromLsFile_SingleFileConflict)
{
	BYTE git_ls_file_u_t_z_output[] = { "M 100644 1f9f46da1ee155aa765d6e379d9d19853358cb07 1	bla.txt\0M 100644 3aa011e7d3609ab9af90c4b10f616312d2be422f 2	bla.txt\0M 100644 56d252d69d535834b9fbfa6f6a633ecd505ea2e6 3	bla.txt\0" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray));
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"bla.txt", testList[0].GetGitPathString());
	EXPECT_STREQ(L"bla.txt", testList[1].GetGitPathString());
	EXPECT_STREQ(L"bla.txt", testList[2].GetGitPathString());
	EXPECT_EQ(1, testList[0].m_Stage);
	EXPECT_EQ(2, testList[1].m_Stage);
	EXPECT_EQ(3, testList[2].m_Stage);
	EXPECT_EQ(0, testList[0].m_Action);
	EXPECT_EQ(0, testList[1].m_Action);
	EXPECT_EQ(0, testList[2].m_Action);
}

TEST(CTGitPath, ParserFromLsFile_DeletedFileConflict)
{
	// file added, modified on branch A, deleted on branch B, merge branch A on B (git status says: "deleted by us")
	BYTE git_ls_file_u_t_z_output[] = { "M 100644 24091f0add7afc47ac7cdc80ae4d3866b2ef588c 1	Neues Textdokument.txt\0M 100644 293b6f6293106b6ebb5d54ad482d7561b0f1c9ae 3	Neues Textdokument.txt\0" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray));
	EXPECT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L"Neues Textdokument.txt", testList[0].GetGitPathString());
	EXPECT_STREQ(L"Neues Textdokument.txt", testList[1].GetGitPathString());
	EXPECT_EQ(1, testList[0].m_Stage);
	EXPECT_EQ(3, testList[1].m_Stage);
	EXPECT_EQ(0, testList[0].m_Action);
	EXPECT_EQ(0, testList[1].m_Action);
}

TEST(CTGitPath, ParserFromLsFile_MultipleFilesConflict)
{
	BYTE git_ls_file_u_t_z_output[] = { "M 100644 0ead9277724fc163fe64e1163cc2ff97d5670e41 1	OSMtracker.sln\0M 100644 7a8a41c7c26d259caba707adea36a8b9ae493c97 2	OSMtracker.sln\0M 100644 dcdd1c25bb0ebfd91082b3de5bd45d3f25418027 3	OSMtracker.sln\0M 100644 cbb00533d69b53e40a97f9bcf1c507a71f3c7353 1	OSMtracker/frmMain.vb\0M 100644 1cbfa36fef1af473884ad2e5820075b581fe33af 2	OSMtracker/frmMain.vb\0M 100644 337331224f438f5f49d5e8a4d4c1bafb66f2e67d 3	OSMtracker/frmMain.vb\0M 100644 786e60a550be11ef8e321222ffe6c0fa0f51f23d 1	OSMtracker/osmTileMap.vb\0M 100644 7fe8b75f56cf0202b4640d74d46240d2ba894115 2	OSMtracker/osmTileMap.vb\0M 100644 53d75915e78d0a53ba05c124d613fd75d625c142 3	OSMtracker/osmTileMap.vb\0" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray));
	EXPECT_EQ(3 * 3, testList.GetCount()); // 3 files are conflicted with 3 stages
	EXPECT_STREQ(L"OSMtracker.sln", testList[0].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker.sln", testList[1].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker.sln", testList[2].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/frmMain.vb", testList[3].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/frmMain.vb", testList[4].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/frmMain.vb", testList[5].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/osmTileMap.vb", testList[6].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/osmTileMap.vb", testList[7].GetGitPathString());
	EXPECT_STREQ(L"OSMtracker/osmTileMap.vb", testList[8].GetGitPathString());
	EXPECT_EQ(1, testList[0].m_Stage);
	EXPECT_EQ(2, testList[1].m_Stage);
	EXPECT_EQ(3, testList[2].m_Stage);
	EXPECT_EQ(1, testList[3].m_Stage);
	EXPECT_EQ(2, testList[4].m_Stage);
	EXPECT_EQ(3, testList[5].m_Stage);
	EXPECT_EQ(1, testList[6].m_Stage);
	EXPECT_EQ(2, testList[7].m_Stage);
	EXPECT_EQ(3, testList[8].m_Stage);
	EXPECT_EQ(0, testList[0].m_Action);
	EXPECT_EQ(0, testList[1].m_Action);
	EXPECT_EQ(0, testList[2].m_Action);
	EXPECT_EQ(0, testList[3].m_Action);
	EXPECT_EQ(0, testList[4].m_Action);
	EXPECT_EQ(0, testList[5].m_Action);
	EXPECT_EQ(0, testList[6].m_Action);
	EXPECT_EQ(0, testList[7].m_Action);
	EXPECT_EQ(0, testList[8].m_Action);
}

TEST(CTGitPath, ParserFromLsFile_NormalRepo)
{
	BYTE git_ls_file_u_t_z_output[] = { "H 100644 73aea48a4ede6d3ca43bc3273c52e81a5d739447 0	README.md\0H 100644 e2780959232e32e0cce8c1866d36b04db85481a6 0	taskxml.xsd\0H 100644 49aa0e1b2e9fcd752767863a1208ad5b5afb254f 0	whitepaper.md\0" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray));
	EXPECT_EQ(3, testList.GetCount());
	EXPECT_STREQ(L"README.md", testList[0].GetGitPathString());
	EXPECT_STREQ(L"taskxml.xsd", testList[1].GetGitPathString());
	EXPECT_STREQ(L"whitepaper.md", testList[2].GetGitPathString());
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_EQ(0, testList[1].m_Stage);
	EXPECT_EQ(0, testList[2].m_Stage);
	EXPECT_EQ(0, testList[0].m_Action);
	EXPECT_EQ(0, testList[1].m_Action);
	EXPECT_EQ(0, testList[2].m_Action);
}

TEST(CTGitPath, ParserFromLsFile_RepoWithSubmodule)
{
	BYTE git_ls_file_u_t_z_output[] = { "H 160000 d4eaf3c5d0994eb0112c17aa3c732022eb9fdf6b 0	ext/gtest\0" };
	CGitByteArray byteArray;
	byteArray.append(git_ls_file_u_t_z_output, sizeof(git_ls_file_u_t_z_output));
	CTGitPathList testList;
	EXPECT_EQ(0, testList.ParserFromLsFile(byteArray));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"ext/gtest", testList[0].GetGitPathString());
	EXPECT_EQ(0, testList[0].m_Stage);
	EXPECT_EQ(0, testList[0].m_Action);
}

TEST(CTGitPath, FillUnRev)
{
	CAutoTempDir tmpDir;

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	g_Git.m_CurrentDir = tmpDir.GetTempDir();

	CTGitPathList testList;
	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(0, testList.GetCount());

	CString fileOne = tmpDir.GetTempDir() + L"\\one";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileOne, L"something"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(0, testList.GetCount());

	CTGitPathList selectList;
	selectList.AddPath(CTGitPath(L"one"));

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	CString gitIgnore = tmpDir.GetTempDir() + L"\\.gitignore";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitIgnore, L""));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());
	EXPECT_STREQ(L"one", testList[1].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitIgnore, L"/one"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_TRUE(CreateDirectory(tmpDir.GetTempDir() + L"\\subdir", nullptr));
	selectList.Clear();
	selectList.AddPath(CTGitPath(L"subdir"));
	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	fileOne = tmpDir.GetTempDir() + L"\\subdir\\one";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(fileOne, L"something"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(2, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());
	EXPECT_STREQ(L"subdir/one", testList[1].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(0, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"subdir/one", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(0, testList.GetCount());

	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(gitIgnore, L"one"));

	EXPECT_EQ(0, testList.FillUnRev(0));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L".gitignore", testList[0].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE));
	EXPECT_STREQ(L"one", testList[0].GetGitPathString());
	EXPECT_STREQ(L"subdir/one", testList[1].GetGitPathString());

	EXPECT_EQ(0, testList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, &selectList));
	EXPECT_EQ(1, testList.GetCount());
	EXPECT_STREQ(L"subdir/one", testList[0].GetGitPathString());
}

TEST(CTGitPath, GetAbbreviatedRename)
{
	CTGitPath test;
	CString newName, oldName;

	// just a failsafe
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"", test.GetAbbreviatedRename());

	oldName = L"B";
	newName = L"A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"B => A", test.GetAbbreviatedRename());

	oldName = L"C/B/A";
	newName = L"A/B/C";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"C/B/A => A/B/C", test.GetAbbreviatedRename());

	oldName = L"B/C";
	newName = L"A/C";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{B => A}/C", test.GetAbbreviatedRename());

	oldName = L"C/B";
	newName = L"C/A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"C/{B => A}", test.GetAbbreviatedRename());

	oldName = L"C/D/E";
	newName = L"C/B/A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"C/{D/E => B/A}", test.GetAbbreviatedRename());

	oldName = L"C/D/A";
	newName = L"D/A";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{C/D => D}/A", test.GetAbbreviatedRename());

	oldName = L"A1/B/C/F";
	newName = L"A2/B/C/F";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{A1 => A2}/B/C/F", test.GetAbbreviatedRename());

	oldName = L"C/D/E";
	newName = L"D/E";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"{C/D => D}/E", test.GetAbbreviatedRename());

	oldName = L"D/E";
	newName = L"D/F/E";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"D/{ => F}/E", test.GetAbbreviatedRename());

	oldName = L"D/F/E";
	newName = L"D/F/F/E";
	test.SetFromGit(newName, &oldName);
	EXPECT_STREQ(L"D/F/{ => F}/E", test.GetAbbreviatedRename());
}

TEST(CTGitPath, HashStashDir)
{
	CAutoTempDir tmpDir;

	CTGitPath path(tmpDir.GetTempDir());
	// no repository -> no stash
	EXPECT_FALSE(path.HasStashDir());

	CAutoRepository repo = nullptr;
	ASSERT_TRUE(git_repository_init(repo.GetPointer(), CUnicodeUtils::GetUTF8(tmpDir.GetTempDir()), false) == 0);

	g_Git.m_CurrentDir = tmpDir.GetTempDir();

	// empty repository no stash
	EXPECT_FALSE(path.HasStashDir());

	CString file(tmpDir.GetTempDir() + L"\\file");
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"don't care"));

	CString output;
	EXPECT_EQ(0, g_Git.Run(L"git add file", &output, CP_UTF8));
	EXPECT_EQ(0, g_Git.Run(L"git commit -m \"test\"", &output, CP_UTF8));
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"don't care even more"));
	EXPECT_EQ(0, g_Git.Run(L"git stash", &output, CP_UTF8));
	EXPECT_TRUE(path.HasStashDir());

	// check for packed stash
	EXPECT_EQ(0, g_Git.Run(L"git pack-refs --all", &output, CP_UTF8));
	EXPECT_TRUE(path.HasStashDir());

	EXPECT_EQ(0, g_Git.Run(L"git stash clear", &output, CP_UTF8));
	EXPECT_FALSE(path.HasStashDir());

	file = tmpDir.GetTempDir() + L"\\.git\\packed-refs";
	EXPECT_TRUE(CStringUtils::WriteStringToTextFile(file, L"# pack-refs with: peeled fully-peeled\nf3a76c72d89aebd63a0346dd92ecafecdc780822 refs/stashNotReal\n"));
	EXPECT_FALSE(path.HasStashDir());
}
