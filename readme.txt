1.收起状态：触碰this，QQ moveOut but 没有焦点(just View)，此时移开鼠标则 moveIn（不打扰用户看视频等）

2.收起状态：点击任务栏图标 or Alt+Tab获取焦点，则moveOut

3.侧边栏状态：将焦点转移 && 鼠标移出QQ区域，则moveIn

4.最小化：达咩 会被瞬间恢复 并moveIN

5.新增左滑手势 侧边栏状态：moveIn 前置悬停状态：moveToSide

6.双击切换active/inActive 指自动hide & moveToSide能力


PS:
1.图片查看器类名 & 样式与QQChat无异 只能通过title区分过滤 并归为QQSubwin & 置顶it（如果好友叫"图片查看"就寄了）[新增"屏幕识图", "翻译"]

2.isAutoHide == false情况下 移动1pxQQ 在切换为true的话：由于qqRect.x()!=0导致无法自动moveIn

3.神奇Bug：双击时可能微小move导致setBGColor(preColor)导致切换颜色后又被切回来（但引发该bug的操作为：点击QQ ->鼠标引出任务栏 ->双击this）（如果省去任务栏 则不会触发）
解决办法：在Move超过Limit后再setBGColor(preColor)

4.getInputFocus() bug:易导致this取得焦点 并成为前台后 QQ的任务栏图标还是选中状态没有更新，导致点击后 直接最小化 而非选中
解决办法：在设置this前台前，先设置QQ为前台，完成更新效果
