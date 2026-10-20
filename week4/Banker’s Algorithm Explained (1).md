# Banker’s Algorithm

Banker’s algorithm is a way of avoiding deadlock. Dijkstra came up with it. The rule is simple: a process only gets the resources it asks for if every process can still finish afterwards.

**Think of a bank.** The bank has a fixed amount of cash and a few customers, each with a credit limit. Before approving a loan, the banker checks one thing: after this loan, can the customers still be served in some order so that each one reaches their limit, finishes and pays the money back? If there is no such order, the loan is refused, even though the bank has the cash right now.

## 1. Core idea: safe vs. unsafe

A state is **safe** if we can line the processes up in a *safe sequence* \<Pa, Pb, …> where each process can get what it still needs from the free resources plus whatever the earlier processes give back when they finish. An **unsafe** state is not a deadlock yet, but it could turn into one. Banker’s algorithm just never lets the system go into an unsafe state.

## 2. Data structures

Say we have `n` processes and `m` resource types. We keep track of four things:

- `Available[m]`: how many instances of each resource are free right now.
- `Max[n][m]`: the maximum each process can ever ask for.
- `Allocation[n][m]`: what each process is holding at the moment.
- `Need[n][m] = Max − Allocation`: what each process might still ask for.

## 3. Safety algorithm

1. Set `Work = Available` and `Finish[i] = false` for all i.
2. Find a process i with `Finish[i] = false` and `Need[i] ≤ Work`. If none exists, go to step 4.
3. Assume it runs to completion and releases everything: `Work = Work + Allocation[i]`, `Finish[i] = true`. Go to step 2.
4. If every `Finish[i]` is true, the state is **safe**. If not, it is **unsafe**.

## 4. Resource-request algorithm

When process Pi makes a request `Requesti`, we do this:

1. If `Requesti > Need[i]`, that is an error, because the process is asking for more than it said it would ever need.
2. If `Requesti > Available`, Pi has to wait because the resources are not there yet.
3. Pretend to grant it: `Available −= Request`, `Allocation[i] += Request`, `Need[i] −= Request`.
4. Run the safety algorithm on the new state. If it is safe, the grant stays. If not, undo it and Pi waits.

## 5. Try it: step through a worked example

Here we have five processes and three resource types (A, B, C). Press **Step** to run the safety algorithm one round at a time and watch what happens.

Work (available): Sequence: –

## 6. Request check: can P1 ask for (1, 0, 2)?

You can test a request on the same starting state. Pick a process, enter the amounts and press check.

Process A B C

## 7. Strengths and limits

**Strengths**

- Avoids deadlock instead of fixing it after it happens.
- Lets more processes run together than strict prevention rules do.

**Limits**

- Every process has to declare its maximum need in advance.
- The number of processes and resources has to stay fixed.
- Each safety check costs O(m·n²), and it can be too cautious sometimes.