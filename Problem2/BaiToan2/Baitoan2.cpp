#include <bits/stdc++.h>

using namespace std;

const double eps = 1e-9;

const int MAXN = 100; // Maximum number of jobs
const int MAXGROUPSIZE = 3;
const double BALANCETHRESH = 4.0/5.0;
const int HMAX = 101;
const int NMAX = 1000;

const int STARTTEMP = 100;
const int DECTEMP = 5;
const int LOOPTIME = 20;

///* Read input

struct job{
    string machine;
    int type;
    double worktime;
    int status;
    vector<int> edge, originEdge;
    vector<int> revEdge;
};

int N, R, NConst;
job jobList[MAXN + 5];

void readInput(){
    //freopen("input4.txt", "r", stdin);

    cin >> N >> R >> NConst;
    for(int i = 1; i <= N; i++){
        int ign;
        cin >> ign >> jobList[i].machine >> ign >> jobList[i].worktime;
        jobList[i].status = 1;
    }

    int M; cin >> M;
    while(M--){
        int u, v; cin >> u >> v;
        jobList[u].edge.push_back(v); jobList[u].originEdge.push_back(v);
        jobList[v].revEdge.push_back(u);
    }

    for(int type = 1; type <= 3; type++){
        cin >> M;
        while(M--){
            string machine; cin >> machine;
            for(int i = 1; i <= N; i++){
                if(jobList[i].machine == machine) jobList[i].type = type;
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

set<int> reachableJobs;
set<int> groupElements;

bool dfs1(int u, int rev){
    if(rev == -1
       && groupElements.find(u) == groupElements.end()
       && reachableJobs.find(u) != reachableJobs.end())
        return true;

    reachableJobs.insert(u * rev);

    if(rev == 1){
        for(int i = 0; i < (int)jobList[u].edge.size(); i++){
            int v = jobList[u].edge[i];
            if(jobList[v].status < 0) v = -jobList[v].status;
            if(reachableJobs.find(v * rev) == reachableJobs.end())
                dfs1(v, rev);
        }
    }
    else{
        for(int i = 0; i < (int)jobList[u].revEdge.size(); i++){
            int v = jobList[u].revEdge[i];
            if(jobList[v].status < 0) v = -jobList[v].status;
            if(reachableJobs.find(v * rev) == reachableJobs.end() && dfs1(v, rev))
                return true;
        }
    }

    return false;
}

bool tooLargeOrBadPair(vector<int> group){
    // not more than MAXGROUPSIZE jobs
    if((int)group.size() > MAXGROUPSIZE) return true;

    // totalTime <= 3.3R
    double totalTime = 0;
    for(int i = 0; i < (int)group.size(); i++)
        totalTime += jobList[group[i]].worktime;
    if((double)totalTime >= 3.35 * (double)R) return true;

    // not more than 2 machine
    set<string> machine;
    for(int i = 0; i < (int)group.size(); i++)
        machine.insert(jobList[group[i]].machine);
    if((int)machine.size() > 2) return true;

    // no 1-1 or 1-2 pair
    set<int> type;
    for(int i = 0; i < (int)group.size(); i++)
        type.insert(jobList[group[i]].type);
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
    reachableJobs.clear();
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
    int jobs;
    double TimeWork;
    double Rj;
};

groupStat calGroupStat(vector<int> group){
    groupStat stat;
    double totalTime = 0;
    int baseWorkers = 0;
    for(int i = 0; i < (int)group.size(); i++){
        totalTime += jobList[group[i]].worktime;
        double workers = jobList[group[i]].worktime / (double)R;
        if(workers < 1.15) baseWorkers ++;
        else if(workers < 2.25) baseWorkers += 2;
        else baseWorkers += 3;
    }

    double actualWorkers = totalTime / (double)R;
    stat.TimeWork = totalTime;
    stat.Rj = actualWorkers;
    if(actualWorkers < 1.15){
        stat.workers = 1;
        stat.workerSaved = baseWorkers - 1;
        stat.balanced = (actualWorkers >= 0.85) ? 1 : 0;
    }
    else if(actualWorkers < 2.25){
        stat.workers = 2;
        stat.workerSaved = baseWorkers - 2;
        stat.balanced = (actualWorkers >= 1.75) ? 1 : 0;
    }
    else{
        stat.workers = 3;
        stat.workerSaved = baseWorkers - 3;
        stat.balanced = (actualWorkers >= 2.65) ? 1 : 0;
    }
    stat.jobs = (int)group.size();
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
            else if(curGroupStat.jobs < curBestGroupStat.jobs) curBestGroup = group;
        }
        //*/
    }
}

void findGroup(int t, vector<int> group, int lastJob){
    if(lastJob != 0){
        jobList[lastJob].status = 2;
        group.push_back(lastJob);
    }

    if(t == MAXGROUPSIZE){
        if(validGroup(group)) cmpGroup(group);
    }
    else{
        findGroup(t + 1, group, 0);
        for(int i = 1; i <= N; i++){
            if(jobList[i].status == 1){
                findGroup(t + 1, group, i);
            }
        }
    }

    if(lastJob != 0){
        jobList[lastJob].status = 1;
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
    		if(jobList[u].status != 1) continue;
    		bool zeroInDegree = true;
    		for(int i = 0; i < (int)jobList[u].revEdge.size(); i++){
    			int x = jobList[u].revEdge[i];
    			if(jobList[x].status <= 0) continue;
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

        jobList[curNode].status = 0;
        for(int j = 1; j < (int)curBestGroup.size(); j++){
            int x = curBestGroup[j];
            for(int k = 0; k < (int)jobList[x].edge.size(); k++)
                jobList[curNode].edge.push_back(jobList[x].edge[k]);
            jobList[x].edge.clear();

            for(int k = 0; k < (int)jobList[x].revEdge.size(); k++)
                jobList[curNode].revEdge.push_back(jobList[x].revEdge[k]);
            jobList[x].revEdge.clear();

            jobList[x].status = -curNode;
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

    for(int i = 0; i < (int)jobList[u].originEdge.size(); i++){
        int v = jobList[u].originEdge[i], vGroupIndex = findGroupIndex(v, sol);
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

//compare Solution current and final
bool cmpSolution2(solution X, solution Y){
    if (X.workers == NConst && Y.workers != NConst) return true;
    else if (X.workers != NConst && Y.workers == NConst) return false;
    else if(X.balance + eps >= BALANCETHRESH && Y.balance + eps < BALANCETHRESH) return true;
    else if(X.balance + eps < BALANCETHRESH && Y.balance + eps >= BALANCETHRESH) return false;
    else if(X.balance + eps >= BALANCETHRESH){
        if(X.workers < Y.workers) return true;
        else if(X.workers > Y.workers) return false;
        else if(fabs(X.balance - Y.balance) < eps) return false;
        else return X.balance > Y.balance;
    }
    else if(fabs(X.balance - Y.balance) < eps) return X.workers < Y.workers;
    else return X.balance > Y.balance;
}

// cmpSolution() returns true if X > Y, false otherwise
bool cmpSolution(solution X, solution Y){
    //if (X.workers == NConst && Y.workers != NConst) return true;
    //else if (X.workers != NConst && Y.workers == NConst) return false;
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
                if(cmpSolution2(tempRes, finalRes) )
                {
                    cout << "\nCac phuong an trung gian: \n";
                    finalRes = tempRes;
                    printSolution(finalRes);

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
                if(cmpSolution2(curRes, finalRes))
                {
                    cout << "\nCac phuong an trung gian: \n";
                    finalRes = curRes;
                    printSolution(finalRes);

                }
            }
            else if(rand() % STARTTEMP + 1 <= temperature) curRes = tempRes;
        }

        //printSolution(finalRes);
        temperature -= DECTEMP;
        choice--;
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
    findSolution();
    cout << "\nPhuong an co so: \n";
    printSolution(finalRes);
    tuneSolution();
    cout << "\nKet qua cuoi cung: \n";
    printSolution(finalRes);

    return 0;
}
