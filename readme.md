# MASQUE: Multi-party Approximate Secure QUEry processing system

We build MASQUE, an MPC-AQP system that evaluates approximate queries on pre-generated samples in the semi-honest two-server model. It is a demo system of paper "Secure Sampling for Approximate Multi-party Query Processing".

## System overview

The system involves several parties,

- any number of data owners (providing data);
- two semi-honest non-colluding servers (performing MPC calculation);
- a client (inquiry an aggregation query).

MASQUE takes a two-stage approach: 

- In the **offline** stage, MASQUE denormalizes the data using existing JOIN protocols if necessary and obtain a flat table in secret-shared form; and then the most improtantly, prepares a batch of samples (uniform sampling without replacement and stratified sampling with specific group-by key $k$);
- In the **online** stage, MASQUE takes the query from the client and constructs a query evaluation circuit with a pre-generated sample, also injects DP noises on query result with a circuit if necessary, and then evaluates the circuit and sents the shares to the client for result reconstruction.

![overview](sysoverview.png)



## Installation guideline

Our work is based on ABY framework. To install our system on Linux, you should clone this repo from Github, and then compile these codes through cmake. Some of the requirements of the system`cmake (version >= 3.13)`, `g++ (vection >= 8) `, `libboost-all-dev (version >= 1.69) `. You can install these requirements by using `sudo apt-get install xxx` in Linux.

```
git clone --recursive https://github.com/sigmod2024/MASQUE.git
mkdir build; cd build
cmake ..
make -j 4
```



## Offline stage

MASQUE implements the batch sample generation in offline stage. It contains the following sampling methods,

- Shuffle sampling(`ShuffleSamples()`)
- With replacement sampling(`UniformWRSamples()`)
- Without replacement sampling (`WoRSamples()`)
- Stratified sampling (It has the same generation function as WoR sampling)

Notice that MASQUE implements each component with a seperate function (i.e., `GenWoRIndices()` generates a batch of random numbers without replacement in range $n$; `GetSamples()` picks samples from the dataset with given index), so you can check each component's cost respectively.

```
[In 'build' directory]
./offline 0 // In server 0
./offline 1 // In server 1

./offline -a [IP address] -p [Port ID] -r [role "0/1"]
```

The last command is a full instructions for use. You can specify IP address and port specifically with `-a` and `-p` . `-r` is a identifier of the two servers. You can also simplify the command by the first two lines in one machine, where the IP address is `127.0.0.1` and port is `8888`.



## Online stage

MASQUE also evaluates the query evaluation circuit in online stage. There are 4 sample queries (Q1, Q1U, Q8, Q9), where details are shown in the paper. And the commands to run online query evaluation is,

```
[In 'build' directory]
./online 0 // In server 0
./online 1 // In server 1

./online -a [IP address] -p [Port ID] -r [role "0/1"]
```

**Rewrite Queries**

MASQUE adopts the two-stage approach, so we also need to rewrite the queries: in the offline phase, we obtain the flat table of given queries by performing several natural joins; in the online phase, we process the query on the flat table.

`Q1 & Q1U`
```
select
	l_returnflag,
	l_linestatus,
	sum(l_quantity) as sum_qty,
	sum(l_extendedprice) as sum_base_price,
	sum(l_extendedprice * (1 - l_discount)) as sum_disc_price,
	sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge,
	avg(l_quantity) as avg_qty,
	avg(l_extendedprice) as avg_price,
	avg(l_discount) as avg_disc,
	count(*) as count_order
from
	lineitem
group by
	l_returnflag,
	l_linestatus;
```
There is no offline processing as the query only involves one relation. `Q1` performs the `group-by` query and `Q1U` adds two selection conditions to sepcify the stratum.
```
select
	sum(l_quantity) as sum_qty,
	sum(l_extendedprice) as sum_base_price,
	sum(l_extendedprice * (1 - l_discount)) as sum_disc_price,
	sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge,
	avg(l_quantity) as avg_qty,
	avg(l_extendedprice) as avg_price,
	avg(l_discount) as avg_disc,
	count(*) as count_order
from
	lineitem
where
	l_returnflag = 'N' and l_linestatus = 'O';
```

`Q8`
```
select 
    o_year, 
    sum(case when s_nationkey = 8 then volume
        else 0 end) / sum(volume) as mkt_share
from (
    select
        s_nationkey,
        extract(year from o_orderdate) as o_year,
        l_extendedprice * (1 - l_discount) as volume
        from
            part, supplier, lineitem, orders, customer
        where
            p_partkey = l_partkey
            and s_suppkey = l_suppkey
            and l_orderkey = o_orderkey
            and o_custkey = c_custkey
            and c_nationkey in (8,9,12,18,21)
            and o_orderdate between date '1995-01-01' and date '1996-12-31'
            and p_type = 'SMALL PLATED COPPER'
    ) as all_nations
group by
    o_year;
```

In the offline phase, we compute the natural joins,
```
select * 
from
    part, supplier, lineitem, orders, customer
where
    p_partkey = l_partkey
    and s_suppkey = l_suppkey
    and l_orderkey = o_orderkey
    and o_custkey = c_custkey
```
The flat table is saved as relation `all_nations`, which has the same size as `lineitem` (due to the foreign-key constraint). And the online phase process two queries,
```
select 
    o_year, 
    sum(volume)
from
    all_nations
where
    c_nationkey in (8,9,12,18,21)
    and o_orderdate between date '1995-01-01' and date '1996-12-31'
    and p_type = 'SMALL PLATED COPPER'
group by
    o_year;
```
and
```
select 
    o_year,
    sum(volume)
from
    all_nations
where
    s_nationkey = 8
    and c_nationkey in (8,9,12,18,21)
    and o_orderdate between date '1995-01-01' and date '1996-12-31'
    and p_type = 'SMALL PLATED COPPER'
group by
    o_year;
```
And further apply the division circuit to calculate the fractions as `mkt_share`.

`Q9`
```
select
    nation,
    sum(amount) as sum_profit
from (
    select
        n_name as nation,
        l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
    from
        part,
        supplier,
        lineitem,
        partsupp,
        nation
    where
        s_suppkey = l_suppkey
        and ps_suppkey = l_suppkey
        and ps_partkey = l_partkey
        and p_partkey = l_partkey
        and s_nationkey = n_nationkey
    ) as profit
group by
    nation;
```
The offline phase process the natural joins as follow,
```
select *
from
    part,
    supplier,
    lineitem,
    partsupp,
    nation
where
    s_suppkey = l_suppkey
    and ps_suppkey = l_suppkey
    and ps_partkey = l_partkey
    and p_partkey = l_partkey
    and s_nationkey = n_nationkey
```
The flat table is saved as relation `profit`. And the online phase process the following queries,
```
select
    n_name as nation,
    sum(l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity) as sum_profit
from
    profit
group by
    nation;
```

