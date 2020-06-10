#include <iostream>
#include <vector>
#include <set>
#include <cassert>

#include "Node.hpp"
#include "Eigen/Dense"

#include "Components/CurrentSource.hpp"
#include "Components/VoltageSource.hpp"
#include "Components/Resistor.hpp"
#include "Components/Inductor.hpp"
#include "Components/Capacitor.hpp"
#include "Components/Component.hpp"

using namespace std;
using namespace Eigen;

vector<Component*> component_list;
vector<Node*> node_list;
double timestep;
double stoptime;

//current statement
set<Node*> s_of_nodes;
//voltage statement
set<Component*> s_of_component;

//calculate the value for the current vector(current statement)
double v_current(Node* node){
    double sum=0;
    for(int j=0;j<component_list.size();j++){
        if((component_list[j]->name()[0]=='I'||component_list[j]->name()[0]=='L') && component_list[j]->node_positive()->name==node->name){
            sum += component_list[j]->current();
        }
        if((component_list[j]->name()[0]=='I'||component_list[j]->name()[0]=='L') && component_list[j]->node_negative()->name==node->name){
            sum -= component_list[j]->current();
        }
    }
    return sum;
}

//calculate the value for the conductance matrix(current statement)
double v_conductance(Node* node1, Node* node2){
    double sum=0;
    if(node1->name==node2->name){
        for(int j=0;j<component_list.size();j++){
            if(component_list[j]->name()[0]=='R' && (component_list[j]->node_positive()->name==node1->name||component_list[j]->node_negative()->name==node1->name)){
                sum += component_list[j]->conductance();
            }
        }
    }else{
        for(int j=0;j<component_list.size();j++){
            if(component_list[j]->name()[0]=='R' && component_list[j]->node_positive()->name==node1->name && component_list[j]->node_negative()->name==node2->name){
                sum += component_list[j]->conductance();
            }
            if (component_list[j]->name()[0]=='R' && component_list[j]->node_positive()->name==node2->name && component_list[j]->node_negative()->name==node1->name){
                sum += component_list[j]->conductance();
            }
        }
    }
    return sum;
}

//input the value for the conductance matrix(voltage statement)
vector<double> v_conductance_input(Component* VS){
    vector<double> res;
    for(int h=0;h<node_list.size();h++){
            res.push_back(0);
        }
    if(VS->node_positive()->name=="0"){
        res[stoi(VS->node_negative()->name.substr(1,3))-1]=-1;
    }else if(VS->node_negative()->name=="0"){
        res[stoi(VS->node_positive()->name.substr(1,3))-1]=1;
    }else{
        res[stoi(VS->node_positive()->name.substr(1,3))-1]=1;
        res[stoi(VS->node_negative()->name.substr(1,3))-1]=-1;
    }
    return res;
}

//input the value for the conductance matrix(current statement)
vector<double> v_conductance_input(Node* node){
    vector<double> res;
    if(node->name=="0"){
        for(int h=0;h<node_list.size();h++){
            res.push_back(v_conductance(node_list[h],node));
        }
    }else{
        for(int h=0;h<node_list.size();h++){
            if (node_list[h]->name==node->name){
                res.push_back(v_conductance(node_list[h],node));
            }else{
                res.push_back(-v_conductance(node_list[h],node));
            }
        }
    }
    return res;
}
 
int main()
{
    //set timestep for component
    for (int i=0;i<component_list.size();i++){
        component_list[i]->set_timestep(timestep);
    }

    //output headers
    cout<<"time";
    for(int i=0;i<node_list.size();i++){
        cout<<",V("<<node_list[i]->name<<")";
    }
    for(int i=0;i<component_list.size();i++){
        cout<<",I("<<component_list[i]->name()<<")";
    }
    cout<<endl;

    //determine the value of s_of_nodes and s_of_component
    for(int j=0;j<component_list.size();j++){
        if(component_list[j]->name()[0]=='V'||component_list[j]->name()[0]=='C'){
            s_of_component.insert(component_list[j]);
            s_of_nodes.erase(component_list[j]->node_positive());
            s_of_nodes.erase(component_list[j]->node_negative());
        }
    }

    for (int i=0;timestep*i<=stoptime;i++){
        cout<<i*timestep;
        //initialize three matrix
        MatrixXd  m_conductance(node_list.size(), node_list.size());
        VectorXd m_current(node_list.size());
        VectorXd m_voltage(node_list.size());

        //value to input row by row
        int row=0;

        //input voltage statement
        set<Component*>::iterator it;
        for (it = s_of_component.begin(); it != s_of_component.end(); ++it) {
            assert(row<=node_list.size());
            m_current(row)=(*it)->voltage();
            vector<double>tmp=v_conductance_input(*it);
            for (int l=0;l<tmp.size();l++){
                m_conductance(row,l)=tmp[l];
            }
            row++;
        }

        set<Node*>::iterator n_it;
        //input current statement
        for (n_it = s_of_nodes.begin(); n_it != s_of_nodes.end(); ++n_it) {
            assert(row<=node_list.size());
            m_current(row)=v_current(*n_it);
            vector<double>tmp=v_conductance_input(*n_it);
            for (int l=0;l<tmp.size();l++){
                m_conductance(row,l)=tmp[l];
            }
            row++;
        }

        //calculation for the voltage matrix
        //cerr << "Here is the conductance matrix:\n" << m_conductance << endl;
        cerr << "Here is the current vector:\n" << m_current << endl;
        m_voltage = m_conductance.colPivHouseholderQr().solve(m_current);
        cerr << "The voltage vector is:\n" << m_voltage << endl;

        //Input & Output Node Voltages
        for (int k=0;k<node_list.size();k++){
            cout<<","<<m_voltage(k);
            node_list[k]->voltage=m_voltage(k);
        }

        for (int k=0;k<component_list.size();k++){
            if(component_list[k]->name()[0]=='V'||component_list[k]->name()[0]=='C'){
                cout<<","<<component_list[k]->source_current(component_list);
            }else{
                cout<<","<<component_list[k]->current();
            }
            
        }

        cout<<endl;

        //progress to the next time interval
        for (int k=0;k<component_list.size();k++){
            component_list[k]->change_time(); 
        }
    }
}


