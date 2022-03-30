1.收起状态：触碰this，QQ moveOut but 没有焦点(just View)，此时移开鼠标则 moveIn（不打扰用户看视频等）

2.收起状态：点击任务栏图标 or Alt+Tab获取焦点，则moveOut

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

PS:
1.图片查看器类名 & 样式与QQChat无异 只能通过title区分过滤 并归为QQSubwin & 置顶it（如果好友叫"图片查看"就寄了）[新增"屏幕识图", "翻译"]

2.isAutoHide == false情况下 移动1pxQQ 在切换为true的话：由于qqRect.x()!=0导致无法自动moveIn

3.神奇Bug：双击时可能微小move导致setBGColor(preColor)导致切换颜色后又被切回来（但引发该bug的操作为：点击QQ ->鼠标引出任务栏 ->双击this）（如果省去任务栏 则不会触发）
解决办法：在Move超过Limit后再setBGColor(preColor)

4.getInputFocus() bug:易导致this取得焦点 并成为前台后 QQ的任务栏图标还是选中状态没有更新，导致点击后 直接最小化 而非选中
解决办法：在设置this前台前，先设置QQ为前台，完成更新效果

5.SendMessage是阻塞函数 可能会挂起 建议改为PostMessage立即返回

6.ShowWindowAsync异步 防止无响应挂起(组合操作不要异步 要等前一步完成)
 fix: 改回同步函数 防止qq还未还原 就移动 导致窗口错位

7.bug：与QQ Follower无关(quit)， 将QQ窗口左右晃动 导致Chrome 最小化后 再晃动恢复 导致Chrome获得TOPMOST，可能由于QQ有TOPMOST属性

8.雷神加速器 会干扰GetKey & AttachThreadInput

9.### GetWindowTextW GetProcessImageFileNameW GetClassNameW：TCHAR数组 用sizeof获取字符数 导致溢出，应用_countof

10.休眠后QQ窗口会向左位移 不能用isQQHideState判断隐藏状态 改用!isQQAllVisible

11.修改isQQSideState为范围，防止move失败导致的细微像素误差

12.Hook鼠标滚轮消息，postEvent发送给this，尽量减少回调函数阻塞， & 只在首次找到qq & moveOut时HOOK，moveIn时UNHOOK，节省资源
增加回调函数checker 过滤pos(将过滤工作前移至回调) 并拦截消息 防止模拟按键时 Ctrl+滚轮导致缩放文本框 对性能影响不大

13.滚轮事件加入限速器(50ms)防止高频模拟按键

14.MousePree时 stopTraceAnima(); //防止move时 timer滞后检测到this 并moveToSide()导致鬼畜
