#include "ObjectiveFun0.h"

int ObjectiveFun0::IsParent(const IPv6Address& srcAddr,double rank)
{
    ParentStructure* rcv = new ParentStructure();
    rcv->ParentId = srcAddr;
    rcv->ParentRank = rank;
    std::vector<ParentStructure*>::iterator it;
    EV<<"judge 0.id"<<countercache[0]->ParentId<<" "<<countercache[0]->ParentRank<<endl;
    for(it = countercache.begin(); it != countercache.end(); it++)
    {
        if((*it)->ParentId == rcv->ParentId)
        {
             if((*it)->ParentRank == rcv->ParentRank)
                 return(EXIST);
             else
                 return(SHOULD_BE_UPDATED);
        }
    }
    return(NOT_EXIST);
}

double ObjectiveFun0::UpdateParent(double counter, int srID, const IPv6Address& srcAddr,double rank, const MACAddress& srcMACAddr)
{
    ParentStructure* rcv = new ParentStructure();
    rcv->srnode = srID;
    rcv->ParentId = srcAddr;
    rcv->ParentMACAddr = srcMACAddr;
    rcv->ParentRank = rank;
    EV<<"rcv mac addr:"<<srcMACAddr<<endl;
    EV<<"rcv parent ipv6 addr:"<<rcv->ParentId<<endl;
    std::vector<ParentStructure*>::iterator it;
    for(it = countercache.begin(); it != countercache.end(); it++)
    {
        if((*it)->srnode == rcv->srnode)
        {
            (*it)->ParentRank = rcv->ParentRank;
            (*it)->ParentMACAddr = rcv->ParentMACAddr;
            (*it)->ParentId = rcv->ParentId;
            break;
        }
    }
    if(it == countercache.end())
    {
        countercache.push_back(rcv);
        EV<<"first time be added in coutercache:"<<rcv->ParentId<<endl;
    }
    for(it = countercache.begin(); it != countercache.end(); it++)
        {
            EV<<(*it)->ParentId<<" ";
        }
    EV<<endl;
    std::stable_sort(countercache.begin(), countercache.end(), counterLessThan);
    EV<<countercache[0]->ParentId<<" "<<countercache[0]->ParentMACAddr<<endl;
    prfparent = countercache[0]->ParentId;
    prfMACparent = countercache[0]->ParentMACAddr;
    EV<<"prf parent id:"<<prfparent<<endl;
    EV<<"prf parent addr:"<<prfMACparent<<endl;

    Rank = countercache[0]->ParentRank + 1;
    PRNodeID = countercache[0]->srnode;
    metrics = Rank;
    return metrics;

}

void ObjectiveFun0::test()
{
    std::vector<ParentStructure*>::iterator it;
    for(it = countercache.begin(); it != countercache.end(); it++)
        {
            EV<<(*it)->ParentId<<" ";
        }
    EV<<endl;
}
bool ObjectiveFun0::counterLessThan(const ParentStructure *a, const ParentStructure *b)
{
        return a->ParentRank < b->ParentRank;
}

