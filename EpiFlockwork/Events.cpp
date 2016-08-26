/* 
 * The MIT License (MIT)
 * Copyright (c) 2016, Benjamin Maier
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
#include "Utilities.h"
#include "Events.h"


const size_t S = 0;
const size_t I = 1;
const size_t R = 2;

using namespace std;

void infect(
                 vector < set < size_t > * > & G, //Adjacency matrix
                 default_random_engine & generator, 
                 uniform_real_distribution<double> & distribution,
                 set < pair < size_t, size_t > > & SI_E, //edge list of SI links
                 vector < size_t > & node_status,
                 set < size_t > & infected
           )
{
    //get element of SI_E that leads to infection
    double r = distribution(generator);
    size_t element = SI_E.size() * r;

    set< pair < size_t, size_t > >::const_iterator edge(SI_E.begin());
    advance(edge,element);

    //get nodes belonging to that edge
    size_t i = (*edge).first;
    size_t j = (*edge).second;

    size_t new_infected;

    if ( ( node_status[i] == I ) && ( node_status[j] == S) )
    {
        new_infected = j;
    } else if ( ( node_status[j] == I ) && ( node_status[i] == S) )
    {
        new_infected = i;
    } else {
        throw domain_error( "There was a non SI link in the set of SI links. This should not happen." );
    }

    //change status of that node
    node_status[new_infected] = I;
    infected.insert(new_infected);

    //loop through the neighbors of i
    for(auto neigh_of_new_I : *G[new_infected] )
    {
        //get pair
        pair <size_t,size_t> current_edge = get_sorted_pair(new_infected,neigh_of_new_I);

        //if the neighbor is infected, then the edge does not belong in SI_E anymore
        if (node_status[neigh_of_new_I] == I)
            SI_E.erase(current_edge);
        //if the neighbor is susceptible, then it belongs in SI_E now
        else if (node_status[neigh_of_new_I] == S)
            SI_E.insert(current_edge);
    }

}
                
void SIS_recover(
                 vector < set < size_t > * > & G, //Adjacency matrix
                 default_random_engine & generator, 
                 uniform_real_distribution<double> & distribution,
                 set < pair < size_t, size_t > > & SI_E, //edge list of SI links
                 vector < size_t > & node_status,
                 set < size_t > & infected
           )
{
    //get element of infected that recovers
    double r = distribution(generator);
    size_t element = infected.size() * r;

    set< size_t >::const_iterator new_recovered_it(infected.begin());
    advance(new_recovered_it,element);

    size_t new_recovered = *new_recovered_it;
    //change status of that node
    node_status[new_recovered] = S;
    infected.erase(new_recovered);

    //loop through the neighbors of i
    for(auto neigh_of_new_S : *G[new_recovered] )
    {
        //get pair
        pair <size_t,size_t> current_edge = get_sorted_pair(new_recovered,neigh_of_new_S);

        //if the neighbor is infected, then the edge belongs in SI_E
        if (node_status[neigh_of_new_S] == I)
            SI_E.insert(current_edge);
        //if the neighbor is susceptible, then it belongs in SI_E now
        else if (node_status[neigh_of_new_S] == S)
            SI_E.erase(current_edge);
    }

}

void rewire(
                 vector < set < size_t > * > & G, //Adjacency matrix
                 double Q,       //probability to connect with neighbors of neighbor
                 default_random_engine & generator, 
                 uniform_real_distribution<double> & distribution,
                 double & mean_degree,
                 set < pair < size_t, size_t > > & SI_E, //edge list of SI links
                 const vector < size_t > & node_status
            )
{
    //choose two nodes
    size_t N = G.size();
    double r1 = distribution(generator);
    double r2 = distribution(generator);
    size_t i,j;
    choose(N,i,j,r1,r2);

    size_t number_of_old_edges = 0;
    size_t number_of_new_edges = 0;

    //loop through the neighbors of i
    for(auto neigh_i : *G[i] )
    {
        //and erase the link to i
        G[neigh_i]->erase(i);
        number_of_old_edges++;

        //check if we erased an SI link
        if ( 
             ( (node_status[i] == I) && (node_status[neigh_i] == S) ) ||
             ( (node_status[i] == S) && (node_status[neigh_i] == I) )
           ) 
        {
            pair <size_t,size_t> current_pair = get_sorted_pair(i,neigh_i);
            SI_E.erase( current_pair );
        }
    } 

    //erase the links from the perspective of i
    G[i]->clear();

    //loop through the neighbors of j
    for(auto neigh_j : *G[j] ) 
    {
        //add as neighbor of i if a random number is smaller than Q
        if ( distribution(generator) < Q )
        {
            G[ neigh_j ]->insert( i );
            G[ i ]->insert( neigh_j );
            number_of_new_edges++;

            //check if we created an SI link
            if ( 
                 ( (node_status[i] == I) && (node_status[neigh_j] == S) ) ||
                 ( (node_status[i] == S) && (node_status[neigh_j] == I) )
               ) 
            {
                pair <size_t,size_t> current_pair = get_sorted_pair(i,neigh_j);
                SI_E.insert( current_pair );
            }
        }
    }

    //add j as neighbor and count additional edge
    G[ j ]->insert( i );
    G[ i ]->insert( j );
    number_of_new_edges++;

    //check if we created an SI link
    if ( 
         ( (node_status[i] == I) && (node_status[j] == S) ) ||
         ( (node_status[i] == S) && (node_status[j] == I) )
       ) 
    {
        pair <size_t,size_t> current_pair = get_sorted_pair(i,j);
        SI_E.insert( current_pair );
    }

    //calculate new number of edges
    mean_degree += 2.0 * ( double(number_of_new_edges) - double(number_of_old_edges) ) / double(N);

}

