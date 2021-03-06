/* 
 * The MIT License (MIT)
 * Copyright (c) 2018, Benjamin Maier
 *
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall 
 * be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-
 * INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN 
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 */

#include "FlockworkPModel.h"

using namespace std;
//namespace osis = SIS;
void
    FlockworkPModel::rewire(
                 double const &_P,       //probability to connect with neighbors of neighbor
                 vector < pair < size_t, size_t > > &edges_in,
                 vector < pair < size_t, size_t > > &edges_out
            )
{
    //choose two nodes
    //double r1 = randuni(*generator);
    //double r2 = randuni(*generator);
    size_t i,j;
    //choose(N,i,j,r1,r2);

    uniform_int_distribution<size_t> node_1(0,N-1); 
    uniform_int_distribution<size_t> node_2(0,N-2);

    i = node_1(*generator);
    j = node_2(*generator);

    if (j>=i)
        j++;

    bool do_rewiring = randuni(*generator) < _P;

    //check if new neighbor is actually an old neighbor
    //and if this is the case return an empty event
    if (not ( do_rewiring and (G[i].find(j) != G[i].end()) ))
    {
        //loop through the neighbors of i
        for(auto neigh_i : G[i] )
        {
            //and erase the link to i
            G[neigh_i].erase(i);
            
            pair < size_t, size_t > current_edge = get_sorted_pair(i,neigh_i);
            edges_out.push_back( current_edge );
        } 

        //erase the links from the perspective of i
        G[i].clear();

        if ( do_rewiring )
        {
            //loop through the neighbors of j
            for(auto neigh_j : G[j] ) 
            {
                G[ neigh_j ].insert( i );
                G[ i ].insert( neigh_j );

                pair < size_t, size_t > current_edge = get_sorted_pair(i,neigh_j);
                edges_in.push_back( current_edge );
            }

            //add j as neighbor and count additional edge
            G[ j ].insert( i );
            G[ i ].insert( j );

            pair < size_t, size_t > current_edge = get_sorted_pair(i,j);
            edges_in.push_back( current_edge );
        }
    }
}

void FlockworkPModel::update_aggregated_network(        
            double const &t,
            vector < pair < size_t, size_t > > &e_in,
            vector < pair < size_t, size_t > > &e_out
        )
{
    if (verbose)
        cout << "===================== updating at time t " << endl;

    for(auto const &edge: e_out)
    {
        if (verbose)
            cout << "edge out: " << edge.first << " " << edge.second << endl;
        double ti = edge_activation_time[edge];

        if (verbose)
            cout << "edge was initialized at t = " << ti << endl;
        double duration = t - ti;

        aggregated_network[edge].value += duration;
        if (verbose)
            cout << "added to aggregated network" << endl;
            cout << "aggregated_network[" << edge.first << " " << edge.second << "].value =" << aggregated_network[edge].value << endl;
    }

    for(auto const &edge: e_in)
    {
        if (verbose)
            cout << "edge in: " << edge.first << " " << edge.second << endl;
       edge_activation_time[edge] = t;
        if (verbose)
            cout << "added activation time" << endl;      
    }
}


void FlockworkPModel::get_rates_and_Lambda(
                    vector < double > &_rates,
                    double &_Lambda
                  )
{
    // delete current rates
    _rates = rates;
    _Lambda = Lambda;
}

void FlockworkPModel::make_event(
                size_t const &event,
                double t,
                vector < pair < size_t, size_t > > &e_in,
                vector < pair < size_t, size_t > > &e_out
               )
{

    if (verbose)
        cout << "attempting to make event " << event << endl;

    e_in.clear();
    e_out.clear();

    if (event==0)
        rewire(1.0,e_in,e_out);
    else if (event == 1)
        rewire(0.0,e_in,e_out);

    if (verbose)
        cout << "rewired." << endl;

    if ( (e_in.size() > 0) or (e_out.size() > 0) )
    {
        if (save_aggregated_network)
            update_aggregated_network(t, e_in, e_out);
        if (save_temporal_network)
        {
            edg_chg.t.push_back(t);
            edg_chg.edges_in.push_back(e_in);
            edg_chg.edges_out.push_back(e_out);
        }

    }

}

void FlockworkPModel::simulate(
                      double t_run_total, 
                      bool reset_all,
                      bool save_network
                      )
{
    if (reset_all)
        reset();

    double t = t0;

    if (save_network)
        edg_chg.t0 = t0;



    while ( t < t_run_total )
    {
       // Draw random number from exponential distribution with mean 1/gamma
       exponential_distribution<double> delta_t(N*gamma); 
       double tau = delta_t(*generator);
       t += tau;

       // let the event happen
       if ( t < t_run_total )
       {
           vector < pair < size_t, size_t > > e_in;
           vector < pair < size_t, size_t > > e_out;
            
           rewire(P,e_in,e_out);

           if ( (e_in.size() > 0) or (e_out.size() > 0) )
           {
               if (save_network)
               {
                   edg_chg.t.push_back(t);
                   edg_chg.edges_in.push_back(e_in);
                   edg_chg.edges_out.push_back(e_out);
               }

                if (save_aggregated_network)
                    update_aggregated_network(t, e_in, e_out);
           }
       }
    }

    if (save_network)
    {
        edg_chg.tmax = t_run_total;
    }
}

