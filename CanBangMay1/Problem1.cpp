#include <bits/stdc++.h>

using namespace std;

const double eps = 1e-9;

const int MAXN = 100; // Maximum number of tasks
const int MAXGROUPSIZE = 3;
const double BALANCETHRESH = 4.0/5.0;
const int HMAX = 101;
const int NMAX = 1000;

const int STARTTEMP = 100;
const int DECTEMP = 5;
const int LOOPTIME = 20;

///* Read input

struct task{
    string machine;
    int type;
    double worktime;
    int status;
    vector<int> edge, originEdge;
    vector<int> revEdge;
};

int N;
double R;
task taskList[MAXN + 5];

void readInput(){
    //freopen("input4.txt", "r", stdin);

    cin >> N >> R;
    for(int i = 1; i <= N; i++){
        int ign;
        cin >> ign >> taskList[i].machine >> ign >> taskList[i].worktime;
        taskList[i].status = 1;
    }

    int M; cin >> M;
    while(M--){
        int u, v; cin >> u >> v;
        taskList[u].edge.push_back(v); taskList[u].originEdge.push_back(v);
        taskList[v].revEdge.push_back(u);
    }

    for(int type = 1; type <= 3; type++){
        cin >> M;
        while(M--){
            string machine; cin >> machine;
            for(int i = 1; i <= N; i++){
                if(taskList[i].machine == machine) taskList[i].type = type;
            }
        }
    }
}

///* Find a good initial solution

struct solution{
    int workers;
    int balancedGroup;
    double balance;
    vector<vector<int> > groups;
};

solution curRes, finalRes;

set<int> reachableTasks;
set<int> groupElements;

bool dfs1(int u, int rev){
    if(rev == -1
       && groupElements.find(u) == groupElements.end()
       && reachableTasks.find(u) != reachableTasks.end())
        return true;

    reachableTasks.insert(u * rev);

    if(rev == 1){
        for(int i = 0; i < (int)taskList[u].edge.size(); i++){
            int v = taskList[u].edge[i];
            if(taskList[v].status < 0) v = -taskList[v].status;
            if(reachableTasks.find(v * rev) == reachableTasks.end())
                dfs1(v, rev);
        }
    }
    else{
        for(int i = 0; i < (int)taskList[u].revEdge.size(); i++){
            int v = taskList[u].revEdge[i];
            if(taskList[v].status < 0) v = -taskList[v].status;
            if(reachableTasks.find(v * rev) == reachableTasks.end() && dfs1(v, rev))
                return true;
        }
    }

    return false;
}

bool tooLargeOrBadPair(vector<int> group){
    // not more than MAXGROUPSIZE tasks
    if((int)group.size() > MAXGROUPSIZE) return true;

    // totalTime <= 3.3R
    double totalTime = 0;
    for(int i = 0; i < (int)group.size(); i++)
        totalTime += taskList[group[i]].worktime;
    if((double)totalTime >= 3.35 * (double)R) return true;

    // not more than 2 machine
    set<string> machine;
    for(int i = 0; i < (int)group.size(); i++)
        machine.insert(taskList[group[i]].machine);
    if((int)machine.size() > 2) return true;

    // no 1-1 or 1-2 pair
    set<int> type;
    for(int i = 0; i < (int)group.size(); i++)
        type.insert(taskList[group[i]].type);
    if((int)machine.size() == 2){
        if((int)type.size() == 1 && type.find(1) != type.end()) return true;
        if(type.find(1) != type.end() && type.find(2) != type.end()) return true;
    }

    //
    return false;
}

bool validGroup(vector<int> group){
    if(tooLargeOrBadPair((group))) return false;

    // no cycle between 2 groups
    reachableTasks.clear();
    groupElements.clear();
    for(int i = 0; i < (int)group.size(); i++){
        groupElements.insert(group[i]);
        dfs1(group[i], 1);
    }
    for(int i = 0; i < (int)group.size(); i++)
        if(dfs1(group[i], -1)) return false;

    return true;
}

struct groupStat{
    int workers;
    int workerSaved;
    int balanced;
    int tasks;
    double TimeWork;
    double Rj;
};

groupStat calGroupStat(vector<int> group){
    groupStat stat;
    double totalTime = 0;
    int baseWorkers = 0;
    for(int i = 0; i < (int)group.size(); i++){
        totalTime += taskList[group[i]].worktime;
        double workers = taskList[group[i]].worktime / (double)R;
        if(workers < 1.15) baseWorkers ++;
        else if(workers < 2.25) baseWorkers += 2;
        else baseWorkers += 3;
    }

    double actualWorkers = totalTime / (double)R;
    stat.TimeWork = totalTime;
    //stat.Rj = actualWorkers;
    if(actualWorkers < 1.15){
        stat.workers = 1;
        stat.workerSaved = baseWorkers - 1;
        //stat.balanced = (actualWorkers >= 0.85) ? 1 : 0;
    }
    else if(actualWorkers < 2.25){
        stat.workers = 2;
        stat.workerSaved = baseWorkers - 2;
        //stat.balanced = (actualWorkers >= 1.75) ? 1 : 0;
    }
    else{
        stat.workers = 3;
        stat.workerSaved = baseWorkers - 3;
        //stat.balanced = (actualWorkers >= 2.65) ? 1 : 0;
    }
    stat.Rj = totalTime/stat.workers;
    stat.balanced = (stat.Rj >= 0.9*R && stat.Rj <= 1.1*R) ? 1 : 0;
    stat.tasks = (int)group.size();
    return stat;
}

vector<int> curBestGroup;

void cmpGroup(vector<int> group){
    groupStat curGroupStat = calGroupStat(group);
    groupStat curBestGroupStat = calGroupStat(curBestGroup);
    if(curGroupStat.workerSaved < curBestGroupStat.workerSaved) return;
    else if(curGroupStat.workerSaved > curBestGroupStat.workerSaved) curBestGroup = group;
    else{
        if(curGroupStat.balanced < curBestGroupStat.balanced) return;
        else if(curGroupStat.balanced > curBestGroupStat.balanced) curBestGroup = group;
        ///* Optional: choose smaller group if all available groups are unbalanced
        else{
            if(curGroupStat.balanced == 1) return;
            else if(curGroupStat.tasks < curBestGroupStat.tasks) curBestGroup = group;
        }
        //*/
    }
}

void findGroup(int t, vector<int> group, int lastTask){
    if(lastTask != 0){
        taskList[lastTask].status = 2;
        group.push_back(lastTask);
    }

    if(t == MAXGROUPSIZE){
        if(validGroup(group)) cmpGroup(group);
    }
    else{
        findGroup(t + 1, group, 0);
        for(int i = 1; i <= N; i++){
            if(taskList[i].status == 1){
                findGroup(t + 1, group, i);
            }
        }
    }

    if(lastTask != 0){
        taskList[lastTask].status = 1;
        group.pop_back();
    }
}

void calSolutionStat(solution* sol){
    (*sol).workers = 0;
    (*sol).balancedGroup = 0;
    for(int i = 0; i < (int)(*sol).groups.size(); i++){
        groupStat stat = calGroupStat((*sol).groups[i]);
        (*sol).workers += stat.workers;
        (*sol).balancedGroup += stat.balanced;
    }
    (*sol).balance = (double)(*sol).balancedGroup / (double)(*sol).groups.size();
}

void findSolution(){
    while(true){
    	bool remain = false;
    	int zeroInDegreeNode = -1;
    	for(int u = 1; u <= N; u++){
    		if(taskList[u].status != 1) continue;
    		bool zeroInDegree = true;
    		for(int i = 0; i < (int)taskList[u].revEdge.size(); i++){
    			int x = taskList[u].revEdge[i];
    			if(taskList[x].status <= 0) continue;
    			zeroInDegree = false;
    		}
    		if(zeroInDegree){
    			zeroInDegreeNode = u;
    			remain = true;
    			break;
    		}
    	}
    	if(!remain) break;

        int curNode = zeroInDegreeNode;
        curBestGroup.clear(); curBestGroup.push_back(curNode);
        vector<int> group;
        findGroup(1, group, curNode);

        taskList[curNode].status = 0;
        for(int j = 1; j < (int)curBestGroup.size(); j++){
            int x = curBestGroup[j];
            for(int k = 0; k < (int)taskList[x].edge.size(); k++)
                taskList[curNode].edge.push_back(taskList[x].edge[k]);
            taskList[x].edge.clear();

            for(int k = 0; k < (int)taskList[x].revEdge.size(); k++)
                taskList[curNode].revEdge.push_back(taskList[x].revEdge[k]);
            taskList[x].revEdge.clear();

            taskList[x].status = -curNode;
        }

        curRes.groups.push_back(curBestGroup);
    }

    calSolutionStat(&curRes);
    finalRes = curRes;
}

///* Tune solution

int visit[MAXN + 5], visitCnt;

int findGroupIndex(int u, solution* sol){
    for(int i = 0; i < (int)(*sol).groups.size(); i++){
        for(int j = 0; j < (int)(*sol).groups[i].size(); j++){
            if((*sol).groups[i][j] == u) return i;
        }
    }
}

bool dfs2(int u, int curGroupIndex, solution* sol){
    visit[u] = visitCnt;
    int uGroupIndex = findGroupIndex(u, sol);

    for(int i = 0; i < (int)(*sol).groups[uGroupIndex].size(); i++){
        int v = (*sol).groups[uGroupIndex][i];
        if(visit[v] != visitCnt && dfs2(v, curGroupIndex, sol)) return true;
    }

    for(int i = 0; i < (int)taskList[u].originEdge.size(); i++){
        int v = taskList[u].originEdge[i], vGroupIndex = findGroupIndex(v, sol);
        if(uGroupIndex != curGroupIndex && vGroupIndex == curGroupIndex) return true;
        if(visit[v] == visitCnt) continue;
        if(dfs2(v, curGroupIndex, sol)) return true;
    }

    return false;
}

bool validSolution(solution* sol){
    for(int i = 0; i < (int)(*sol).groups.size(); i++)
        if(tooLargeOrBadPair((*sol).groups[i])) return false;

    // no cycle between 2 groups
    for(int i = 0; i < (int)(*sol).groups.size(); i++){
        visitCnt++;
        if(dfs2((*sol).groups[i][0], i, sol)) return false;
    }

    return true;
}

// cmpSolution() returns true if X > Y, false otherwise
bool cmpSolution(solution X, solution Y){
    if(X.balance + eps >= BALANCETHRESH && Y.balance + eps < BALANCETHRESH) return true;
    else if(X.balance + eps < BALANCETHRESH && Y.balance + eps >= BALANCETHRESH) return false;
    else if(X.balance + eps >= BALANCETHRESH){
        if(X.workers < Y.workers) return true;
        else if(X.workers > Y.workers) return false;
        else if(fabs(X.balance - Y.balance) < eps) return false;
        else return X.balance > Y.balance;
    }
    else if(fabs(X.balance - Y.balance) < eps) return X.workers < Y.workers;
    else return X.balance > Y.balance;


    /*
    float H;
    H = max(X.balance, Y.balance);
    float fX, fY;
    fX = (float)(X.balance >= 80) * (HMAX * X.workers - X.balance - NMAX*HMAX) + (float)(X.balance < 80) * (X.workers - X.balance * NMAX);
    fY = (float)(Y.balance >= 80) * (HMAX * Y.workers - Y.balance - NMAX*HMAX) + (float)(Y.balance < 80) * (Y.workers - Y.balance * NMAX);
    if (fX < fY) return true;
    else return false;
    */

}

void printSolution(solution a);

solution getNeighbor(int choice){
    vector<solution> solList;
    for(int i = 0; i < (int)curRes.groups.size(); i++){
        for(int j = 0; j < (int)curRes.groups[i].size(); j++){
            for(int k = 0; k < (int)curRes.groups.size()+1; k++){
                solution tempRes = curRes;
                if(k == i) continue;
                else if(k < (int)curRes.groups.size()){
                    tempRes.groups[k].push_back(tempRes.groups[i][j]);
                    tempRes.groups[i].erase(tempRes.groups[i].begin()+j);
                    if(tempRes.groups[i].empty()) tempRes.groups.erase(tempRes.groups.begin()+i);
                }
                else if((int)curRes.groups[i].size() > 1){
                    vector<int> tempVec; tempVec.push_back(tempRes.groups[i][j]);
                    tempRes.groups.push_back(tempVec);
                    tempRes.groups[i].erase(tempRes.groups[i].begin()+j);
                }
                else continue;

                // Check for better solution
                if(!validSolution(&tempRes)) continue;
                calSolutionStat(&tempRes);
                if(cmpSolution(tempRes, finalRes))
                {
                    //cout << "\nCac phuong an trung gian: \n";
                    //printSolution(finalRes);
                    finalRes = tempRes;
                }
                solList.push_back(tempRes);
            }
        }
    }

    // Choose "choice" best neighbors and randomly take one
    if(solList.empty()) return curRes;
    sort(solList.begin(), solList.end(), cmpSolution);
    while((int)solList.size() > choice) solList.pop_back();
    return solList[rand() % (int)solList.size()];
}



void tuneSolution(){
    ///* Simulated annealing
    srand(time(NULL));
    int temperature = STARTTEMP;
    int choice = (STARTTEMP - 1) / DECTEMP + 1;

    while(temperature > 0){
        for(int i = 1; i <= LOOPTIME; i++){
            solution tempRes = getNeighbor(choice);
            if(tempRes.groups == curRes.groups) break;
            if(cmpSolution(tempRes, curRes)){
                curRes = tempRes;
                //cout << "\nCac phuong an trung gian: \n";
                 //   printSolution(tempRes);
                if(cmpSolution(curRes, finalRes))
                {
                    //cout << "\nCac phuong an trung gian: \n";
                    //printSolution(finalRes);
                    finalRes = curRes;
                }
            }
            else if(rand() % STARTTEMP + 1 <= temperature)
            {
                //cout << "\nCac phuong an trung gian: \n";
                //    printSolution(tempRes);
                curRes = tempRes;
            }
        }

        //printSolution(finalRes);
        temperature -= DECTEMP;
        choice--;
    }
    //*/

    /* Optimize only after simulated annealing
    while(true){
        bool hasBetterSolution = false;
        for(int i = 0; i < (int)finalRes.groups.size(); i++){
            for(int j = 0; j < (int)finalRes.groups[i].size(); j++){
                for(int k = 0; k < (int)finalRes.groups.size()+1; k++){
                    solution tempRes = finalRes;
                    if(k == i) continue;
                    else if(k < (int)finalRes.groups.size()){
                        tempRes.groups[k].push_back(tempRes.groups[i][j]);
                        tempRes.groups[i].erase(tempRes.groups[i].begin()+j);
                        if(tempRes.groups[i].empty()) tempRes.groups.erase(tempRes.groups.begin()+i);
                    }
                    else if((int)tempRes.groups[i].size() > 1){
                        vector<int> tempVec; tempVec.push_back(tempRes.groups[i][j]);
                        tempRes.groups.push_back(tempVec);
                        tempRes.groups[i].erase(tempRes.groups[i].begin()+j);
                    }
                    else continue;
                    // Check for better solution
                    if(!validSolution(&tempRes)) continue;
                    calSolutionStat(&tempRes);
                    if(cmpSolution(tempRes, finalRes)){
                        hasBetterSolution = true;
                        finalRes = tempRes;
                        break;
                    }
                }
                if(hasBetterSolution) break;
            }
            if(hasBetterSolution) break;
        }
        if(!hasBetterSolution) break;
    }
    //*/
}

///* Print final solution

void printSolution(solution finalRes){
    int totalWorkers = 0;
    int totalWorkerSaved = 0;
    int balancedGroups = 0;

    for(int i = 0; i < (int)finalRes.groups.size(); i++)
        sort(finalRes.groups[i].begin(), finalRes.groups[i].end());
    sort(finalRes.groups.begin(), finalRes.groups.end());

    cout << (int)finalRes.groups.size() << " groups" << endl;
    for(int i = 0; i < (int)finalRes.groups.size(); i++){
        cout << "Group " << i + 1 << ":";
        for(int j = 0; j < (int)finalRes.groups[i].size(); j++){
            cout << " " << finalRes.groups[i][j];
        }

        groupStat stat = calGroupStat(finalRes.groups[i]);
        totalWorkers += stat.workers;
        balancedGroups += stat.balanced;
        totalWorkerSaved += stat.workerSaved;
        cout << " -- W: " << stat.workers << ", S: " << stat.workerSaved << ", T: " << stat.TimeWork <<", Nc: " << stat.Rj ;
        if(stat.balanced == 1) cout << " YES" << endl; else cout << " NO" << endl;
    }

    cout << totalWorkers << " workers, " << totalWorkerSaved << " saved, ";
    cout << 100.0 * (double)balancedGroups / (double)finalRes.groups.size() << "%" << endl;
}

int main(){
    readInput();
    clock_t start = clock();
    findSolution();
    cout << "\nPhuong an co so: \n";
    printSolution(finalRes);
    tuneSolution();
    cout << "\nKet qua cuoi cung cua R = " << R << " : \n" ;
    printSolution(finalRes);
    clock_t finish = clock();
	double duration = (double)(finish - start) / CLOCKS_PER_SEC;
	cout <<"\n\n\n";
	printf("Thoi gian thuc thi: %.2lf", duration);
	system("pause");
    return 0;
}
