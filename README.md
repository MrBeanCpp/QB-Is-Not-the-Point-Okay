# QB Is Not the Point Okay

#### 介绍
QB Is Not the Point Okay（QINPO：ちんぽう：珍宝）  
or QQFolower  
跟随 Tencent QQ（旧版） 并赋予其**侧边隐藏**能力

### 效果展示

<img src="images/ForREADME/GIF.gif" alt="GIF" style="zoom:67%;" />

### 具体功能

1.收起状态：触碰this(指的是本程序窗口)，QQ moveOut but 没有焦点(just View)，此时移开鼠标则 moveIn（不打扰用户看视频等）

2.收起状态：点击任务栏图标 or `Alt+Tab`获取焦点，则moveOut

3.侧边栏状态：将焦点转移 && 鼠标移出QQ区域，则moveIn

4.最小化：达咩 会被瞬间恢复 并moveIN

5.新增左滑手势 侧边栏状态：moveIn 前置悬停状态：moveToSide

6.双击切换active/inActive 指自动hide & moveToSide能力

7.滚轮切换聊天窗口(强制qq获取焦点)
增加qq左端区域对滚轮的响应（Hook）
单个窗口会导致左端部分区域滚动时->Ctrl+Tab导致文本框TABTABTABTABTAB
改为回调函数过滤pos 并拦截消息 防止模拟按键时 Ctrl+滚轮导致缩放文本框

8.拖拽this quit
改为 y方向随意移动 x方向(正方向)达到一定速度才能突破吸附
y方向加上[上下]限制 setBGColor在isNotStick状态生效（否则容易误改颜色）

9.新增拖拽文件至this时弹出QQ

10.游戏状态下自动隐藏

11.消息coming时，左侧弹出好友头像 as Tip
