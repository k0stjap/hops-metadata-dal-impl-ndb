/*
 * Hops Database abstraction layer for storing the hops metadata in MySQL Cluster
 * Copyright (C) 2015  hops.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
package io.hops.metadata.ndb.wrapper;

import com.mysql.clusterj.*;
import com.mysql.clusterj.query.QueryBuilder;
import io.hops.exception.StorageException;
import io.hops.metadata.ndb.cache.DTOCache;
import io.hops.metadata.ndb.cache.DTOCacheImpl;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.Collection;
import java.util.List;

public class HopsSession {
  private final Log LOG = LogFactory.getLog(HopsSession.class);

  private final Session session;
  private DTOCache dtoCache = null;

  public HopsSession(Session session) {
    this.session = session;
  }

  public boolean isCachedEnabled() {
    return dtoCache != null;
  }

  public HopsQueryBuilder getQueryBuilder() throws StorageException {
    try {
      QueryBuilder queryBuilder = session.getQueryBuilder();
      return new HopsQueryBuilder(queryBuilder);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> HopsQuery<T> createQuery(HopsQueryDomainType<T> queryDefinition)
      throws StorageException {
    try {
      Query<T> query =
          session.createQuery(queryDefinition.getQueryDomainType());
      return new HopsQuery<T>(query);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> T find(Class<T> aClass, Object o) throws StorageException {
    try {
      return session.find(aClass, o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void createDTOCache() {
    dtoCache = new DTOCacheImpl();
  }

  private void assertCacheEnabled() throws StorageException {
    if (dtoCache == null) {
      throw new StorageException("DTO cache is disabled for this session");
    }
  }

  public void registerType(Class type, int cacheSize) throws StorageException {
    assertCacheEnabled();
    dtoCache.registerType(type, cacheSize);
  }

  public void deregisterType(Class type) throws StorageException {
    assertCacheEnabled();
    dtoCache.deregisterType(type);
  }

  public <T> boolean putToCache(Class<T> type, T element) throws StorageException {
    assertCacheEnabled();

    return dtoCache.put(type, element);
  }

  public List<Class> getNotFullTypes() throws StorageException {
    return dtoCache.getNotFullTypes();
  }

  public <T> T newCachedInstance(Class<T> aClass) throws StorageException {
    if (dtoCache != null && dtoCache.containsType(aClass)) {
      T instance = dtoCache.get(aClass);
      if (instance != null) {
        return instance;
      }
    }

    return newInstance(aClass);
  }

  public <T> T newInstance(Class<T> aClass) throws StorageException {
    try {
      return session.newInstance(aClass);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> T newInstance(Class<T> aClass, Object o) throws StorageException {
    try {
      return session.newInstance(aClass, o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> T makePersistent(T t) throws StorageException {
    try {
      return session.makePersistent(t);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> T load(T t) throws StorageException {
    try {
      return session.load(t);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public Boolean found(Object o) throws StorageException {
    try {
      return session.found(o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void persist(Object o) throws StorageException {
    try {
      session.persist(o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public Iterable<?> makePersistentAll(Iterable<?> iterable)
      throws StorageException {
    try {
      return session.makePersistentAll(iterable);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> void deletePersistent(Class<T> aClass, Object o)
      throws StorageException {
    try {
      session.deletePersistent(aClass, o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void deletePersistent(Object o) throws StorageException {
    try {
      session.deletePersistent(o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void remove(Object o) throws StorageException {
    try {
      session.remove(o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> int deletePersistentAll(Class<T> aClass) throws StorageException {
    try {
      return session.deletePersistentAll(aClass);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void deletePersistentAll(Iterable<?> iterable)
      throws StorageException {
    try {
      session.deletePersistentAll(iterable);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void updatePersistent(Object o) throws StorageException {
    try {
      session.updatePersistent(o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void updatePersistentAll(Iterable<?> iterable)
      throws StorageException {
    try {
      session.updatePersistentAll(iterable);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public <T> T savePersistent(T t) throws StorageException {
    try {
      return session.savePersistent(t);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public Iterable<?> savePersistentAll(Iterable<?> iterable)
      throws StorageException {
    try {
      return session.savePersistentAll(iterable);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public HopsTransaction currentTransaction() throws StorageException {
    try {
      Transaction transaction = session.currentTransaction();
      return new HopsTransaction(transaction);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void close() throws StorageException {
    try {
      session.close();
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public boolean isClosed() throws StorageException {
    try {
      return session.isClosed();
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void flush() throws StorageException {
    try {
      session.flush();
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void setPartitionKey(Class<?> aClass, Object o)
      throws StorageException {
    try {
      session.setPartitionKey(aClass, o);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void setLockMode(LockMode lockMode) throws StorageException {
    try {
      session.setLockMode(lockMode);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public void markModified(Object o, String s) throws StorageException {
    try {
      session.markModified(o, s);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }

  public String unloadSchema(Class<?> aClass) throws StorageException {
    try {
      return session.unloadSchema(aClass);
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }
  
  public <T> void release(T t)  throws StorageException {
    try {
      if(t!=null){
        session.release(t);
      }
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }
  
  public <T> void release(Collection<T> t)  throws StorageException {
    try {
      if(t!=null){
        for(T dto : t)  {
          session.release(dto);
        }
      }
    } catch (ClusterJException e) {
      throw HopsExceptionHelper.wrap(e);
    }
  }
}
