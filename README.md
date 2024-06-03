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

## 学习资料

- 事件驱动编程: https://stdcxx.apache.org/doc/stdlibug/11-3.html

## 策略设计

### V1

![](doc/img/v1.svg)

## 评估函数

### V0

```
运输成本 + 快包裹运输时间 * 10 + 慢包裹运输时间 * 5
```
