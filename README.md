# PoisssonRecon
PoissonRecon

使用方法:
1. 使用cloudcompare进行normal计算,保存点云文件
2. 设置configSL.txt
  - input_dir:带有法向的ply文件夹
  - output_dir:输出结果路径
  - depth:八叉树的数目,数值越大越精细
  - num_threads:使用线程数目
  - trim:数值越大,越倾向于去除大面片.
