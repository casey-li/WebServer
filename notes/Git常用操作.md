# 目录

* [1. 工作原理](#1-工作原理)
* [2. 具体操作](#2-具体操作)
  * [2.1 创建版本库](#21-创建版本库)
  * [2.2 把文件添加到版本库中](#22-把文件添加到版本库中)
  * [2.3 版本回退](#23-版本回退)
  * [2.4 撤销修改和删除文件操作](#24-撤销修改和删除文件操作)
    * [2.4.1 撤销修改](#241-撤销修改)
    * [2.4.2 删除文件](#242-删除文件)
  * [2.5 远程仓库](#25-远程仓库)
    * [2.5.1 从远程库克隆](#251-从远程库克隆)
  * [2.6 创建与合并分支](#26-创建与合并分支)
  * [2.7 bug 分支](#27-bug-分支)
  * [2.8 多人协作](#28-多人协作)
    * [2.8.1 推送分支](#281-推送分支)
    * [2.8.2 抓取分支](#282-抓取分支)
  

Git是分布式版本控制系统，只能跟踪文本文件的改动，比如txt文件，网页，所有程序的代码等

版本控制系统可以告诉你每次的改动，但是图片，视频这些二进制文件，虽能也能由版本控制系统管理，但没法跟踪文件的变化，只能把二进制文件每次改动串起来，也就是知道图片从1kb变成2kb，但是到底改了啥，版本控制也不知道

# 1. 工作原理
![](https://pic2.zhimg.com/v2-3bc9d5f2c49a713c776e69676d7d56c5_r.jpg)

- Workspace：工作区
- Index / Stage：暂存区
- Repository：仓库区（或本地仓库）
- Remote：远程仓库

工作区：就是你在电脑上看到的目录，比如目录里的文件(.git隐藏目录版本库除外)。或者以后需要再新建的目录文件等等都属于工作区范畴

版本库(Repository)：工作区有一个隐藏目录.git,这个不属于工作区，这是版本库。其中版本库里面存了很多东西，其中最重要的就是stage(暂存区)，还有Git为我们自动创建了第一个分支master,以及指向master的一个指针HEAD


# 2. 具体操作
## 2.1 创建版本库
版本库又名仓库，英文名repository，可以简单的理解为一个目录
这个目录里面的所有文件都可以被Git管理起来，每个文件的修改，删除，Git都能跟踪，以便任何时刻都可以追踪历史，或者在将来某个时刻还可以将文件”还原”

```shell
git init
```

## 2.2 把文件添加到版本库中

```
// 假设新建了一个 readme.md 文件
git add readme.md // 将 readme.md 添加到暂存区
git commit -m "readme.md提交" // 用命令 git commit 告诉Git，把文件提交到仓库

// 查看状态
// 可以显示是否还有文件未提交，哪些文件被修改，哪些被删除...
git status 

// 假设修改了 readme.md 文件中的内容
// 查看具体修改了什么内容
git diff readme.md 
// 若没问题就可以 git add, git commit 了
```

## 2.3 版本回退

```
// 查看修改历史记录
git log
// 显示精简版本的历史修改记录
git log --pretty=oneline 

// 版本回退
git reset --hard HEAD^ // 回退到上一个版本，上上个版本 (HEAD^^); 回退到前100个版本 (HEAD ~100)

// 假设当前版本为第四版，并且回到了第二版
// 版本回退后，git log 就不会显示第三版第四版的信息了
git reflog // 即使版本回退了，仍能显示完整的版本
git reset --hard 版本号 // 可以回到指定的版本，包括最新版本
```

## 2.4 撤销修改和删除文件操作

### 2.4.1 撤销修改
```
// 方式一、如果知道要删掉那些内容的话，直接手动更改去掉那些需要的文件，然后add添加到暂存区，最后commit

// 方式二、直接恢复到上一个版本。 git reset --hard HEAD^

// 方式三、
git status //返回的结果会告诉你如何撤销

/*
$ git status
On branch main
Your branch is up to date with 'origin/main'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
        modified:   readme.md

no changes added to commit (use "git add" and/or "git commit -a")
*/

git restore readme.md // 把 readme.md 在工作区做的修改全部撤销
```

### 2.4.2 删除文件

```
// 假设新建了一个文件 a.txt，并 add 和 commit 了
rm a.txt
git status // 可以看到 deleted: a.txt

// 此时有两个选项，直接 commit 删除或者从版本库中回复被删除的文件 (git restore a.txt 再 commit)
```

## 2.5 远程仓库

因为本地 Git 仓库和 github 仓库之间的传输是通过 SSH 加密的，所以首先应进行如下设置

1. 创建 SSH Key。检查用户目录下的 .ssh 目录，如果有这个目录检查是否有文件 id_rsa (私钥) 和 id_rsa.pub (公钥)。若没有的话在命令行输入 `ssh-keygen -t rsa –C “youremail@example.com”`
2. 登录 github 添加 SSH Key，输入本机的公钥即可

```
// 假设此时新建了一个 GitHub 仓库，它会告诉你如何处理
// 将本地仓库的内容推送到 GitHub 仓库
git remote add origin https://github.com/...
git push -u origin master

```
由于远程库是空的，我们第一次推送 master 分支时，加上了 `–u` 参数，Git不但会把本地的 master 分支内容推送的远程新的 master 分支，还会把本地的 master 分支和远程的 master  分支关联起来，在以后的推送或者拉取时就可以简化命令

之后只要本地做了修改，就可以通过 `git push origin master` 将本地 master 分支的最新修改推送到github上

### 2.5.1 从远程库克隆

`git clone https://github.com/...`

## 2.6 创建与合并分支

```
git checkout -b dev // 创建并切换分支
git branch dev // 仅创建分支
git checkout dev // 切换分支为 dev

git branch // 查看分支，会列出所有分支，当前分支前有一个 *

// 假设当前在 dev 分支上，并在 readme.md 中增加了一行数据
git add readme.md
git commit -m "dev分支在 readme.md 中新增一行数据"

git checkout master // 切换回 master 分支再查看 readme.md，文件未被修改
git merge dev // 在 master 分支上可以把 dev 分支上的内容合并到 master 分支上

git branch -d dev // 删除 dev 分支
```
`git merge` 命令用于合并指定分支到当前分支上。若显示 `Fast-forward` ，Git告诉我们，这次合并是“快进模式”，也就是直接把 master 指向 dev 的当前提交，所以合并速度非常快

**总结**
- 查看分支：`git branch`
- 创建分支：`git branch name`
- 切换分支：`git checkout name`
- 创建+切换分支：`git checkout –b name`
- 合并某分支到当前分支：`git merge name`
- 删除分支：`git branch –d name`

***解决冲突***

```
// 新建分支，并在 readme.md 中添加一行 aaaa。返回 master 分支，在未合并前在 readme.md 中添加一行 bbbb
git checkout -b sub
git add readme.md
git commit -m "sub 添加内容 aaaa"

git checkout master
git add readme.md
git commit -m "master 添加内容 bbbb"

git merge sub // 产生冲突，Automatic merge failed; fix conflicts and then commit the result.
cat readme.md
/*
1111
2222
<<<<<<< HEAD
bbbb
=======
aaaa
>>>>>>> sub
*/
```

Git 用<<<<<<<，=======，>>>>>>>标记出不同分支的内容，其中 <<<<<<<HEAD 是指主分支修改的内容，>>>>>sub 是指 sub 上修改的内容。需要手动修改，然后再 `add`, `commit`

此时使用`git log` 查看历史信息会发现没有 sub 分支的版本

***分支管理策略***

通常合并分支时，git一般使用 "Fast forward" 模式，在这种模式下，删除分支后，会丢掉分支信息，可以使用带参数 `–-no-ff` 来禁用 "Fast forward" "模式

```
git checkout -b dev
git add a.txt
git commit -m "dev 分支在 a.txt 中新增内容 aaaa"
git checkout master
git merge --no-ff -m "merge with no-ff" dev
git branch -d dev // 显示版本号，如 abcdef
git log // 发现存在版本 abcdef "dev 分支在 a.txt 中新增内容 aaaa"
```

分支策略：master 主分支应该是非常稳定的，也就是用来发布新版本，一般情况下不允许在上面干活，干活一般情况下在新建的 dev 分支上干活，干完后，比如上要发布，或者说dev分支代码稳定后可以合并到主分支master上来


## 2.7 bug 分支

在开发中，会经常碰到bug问题，那么有了bug就需要修复，在Git中，分支是很强大的，每个bug都可以通过一个临时分支来修复，修复完成后，合并分支，然后将临时的分支删除掉

但是，当前 dev 分支上的工作可能还没有做完，还不能进行提交。做完当前工作可能还需要一周但是 bug 仅需要一小时就可以解决，此时可以先“隐藏”当前工作现场 `git stash`，再查看 `git status` 就会发现什么也没有。

之后就可以在 master 分支上新建 sub 分支处理bug，`add`,`commit`修复完成后切回 master 分支合并并删除 sub 分支

切回 dev 分支，`git stash list` 可以查看之前的工作现场。恢复工作现场有两种方式

```
// 方法一、
git stash apply // 恢复现场，但是 stash 中的内容并不删除
git stash drop // 删除 stash 中的内容

// 方法二、
git stash pop // 恢复现场的同时把 stash 里的内容删除
```

## 2.8 多人协作

当你从远程库克隆时候，实际上Git自动把本地的master分支和远程的master分支对应起来了，并且远程库的默认名称是origin

- 查看远程库的信息 使用 `git remote`
- 查看远程库的详细信息 使用 `git remote –v`

```
$ git remote -v
origin  https://github.com/casey-li/WebServer.git (fetch) // 抓取
origin  https://github.com/casey-li/WebServer.git (push) // 推送
```

### 2.8.1 推送分支

推送分支就是把该分支上所有本地提交到远程库中，推送时，要指定本地分支，这样，Git就会把该分支推送到远程库对应的远程分支上 `git push origin master`

推送到其他分支，比如dev分支上，我们还是那个命令 `git push origin dev`

那么一般情况下，那些分支要推送呢？

master分支是主分支，因此要时刻与远程同步。一些修复bug分支不需要推送到远程去，可以先合并到主分支上，然后把主分支master推送到远程去

### 2.8.2 抓取分支

多人协作时，大家都会往master分支上推送各自的修改。现在我们可以模拟另外一个同事，可以在另一台电脑上（注意要把SSH key添加到github上）或者同一台电脑上另外一个目录克隆，新建一个目录名字叫testgit2

1. 首先要把 dev 分支推送到远程去
2. 进入testgit2 目录，克隆远程的库到本地
3. 现在我们的小伙伴要在 dev 分支上做开发，就必须把远程的 origin 的 dev 分支到本地来，于是可以使用命令创建本地 dev 分支：`git checkout –b dev origin/dev`
4. 现在小伙伴们就可以在 dev 分支上做开发了，开发完成后把 dev 分支推送到远程库 (`add`,`commit`,`git push origin dec`)
5. 小伙伴们已经向 origin/dev 分支上推送了提交，而我在我的目录文件下也对同样的文件同个地方作了修改，也试图推送到远程库 (`git checkout dev`,`add`,`commit`,`git push origin dec`)。此时最后一步会产生错误(不同的人推同样的文件，修改同个文件的同个地方报错)

解决方法：先用 `git pull` 把最新的提交从 origin/dev 抓下来，然后在本地合并，解决冲突，再推送

直接 `git pull` 也会失败。原因是没有指定本地 dev 分支与远程 origin/dev 分支的链接

根据提示，设置 dev 和 origin/dev 的链接 `git branch --set-upstream dev origin/dev`

再 `git pull` 成功，但是返回合并有冲突，需要手动解决。解决的方法和分支管理中的 解决冲突完全一样。解决后，提交，再 `git push origin dev`