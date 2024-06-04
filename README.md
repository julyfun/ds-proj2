## 运行方法

```
git clone git@github.com:julyfun/ds-proj2.git
cd ds-proj2
git submodule update --init # 获取 fmt 库
mkdir build
cd build
cmake ..
make
```

编译完成，接下来，我们有若干测试点，你可以选择一个测试。

- 测试简单路径（见 main.cpp 中的 `TEST("simple")`）： `./run --dt-test-case=simple -s`
- 测试 dijkstra 函数是否正常运作： `./run --dt-test-case=dijkstra -s`

## Roadmap

https://docs.qq.com/sheet/DWHJOZHRLZE9YaUpt?tab=BB08J2

## 学习资料

- 事件驱动编程: https://stdcxx.apache.org/doc/stdlibug/11-3.html

## 策略设计

### V1

![](doc/img/v1.svg)

V1 问题:

- 同一个站点的 TryProcess 可能会平方增长，log 爆炸

### V1.a (deprecated)

记录全局 floyd，且记录是否预定了 TryProcessOne。

- 静态

### V1.b

`dijkstra enhanced` 当有站点 buffer 比较满的时候，不选择该站点。若生成路径失败，则采用原始 `dijkstra`

ref: https://www.mdpi.com/2071-1050/14/16/10367

- 动态

### V2

- 记录 package 的去向，在 dijkstra 中估计等待时间，加入代价估计中。可设定线性参数来估计未来代价，假定到达时间之前站点疯狂处理包裹
- 优化站点 Try 失败后添加 Try 的策略，减少事件数量

### V2.b

- 优先取 EXPRESS

### V2 问题

- 跑的太慢, 1152 行数据跑了 2~3s（开 O2 以后 600ms ~ 900ms）
- 会有死包裹

### V3

定时采样，用滤波器记录历史站点的拥堵情况，判断拥堵的可能性。

# 评估函数

### V0

```
运输成本 + 快包裹运输时间 * 10 + 慢包裹运输时间 * 5
```

问题：不能防止死包裹

### V1
